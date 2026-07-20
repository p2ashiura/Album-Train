// albumtrain.cpp
// foobar2000 SDK 2025-03-07 / Columns UI 3.5.0 対応
// アートワークを取得・表示するUIパネル（container_uie_window_v3 版）

#include "stdafx.h"

#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <random>
#include <ctime>
#include <foobar2000/SDK/playback_control.h>
#include <foobar2000/SDK/play_callback.h>
#include <foobar2000/SDK/album_art.h>
#include <foobar2000/SDK/library_manager.h>
#include <foobar2000/SDK/contextmenu_manager.h>   // ← contextmenu.h から変更
#include <foobar2000/SDK/console.h>   // ← 診断ログ出力用（v3.5.3診断ビルド）
#include <libPPUI/win32_op.h>
#include <helpers/BumpableElem.h>
#include "columns_ui-sdk/ui_extension.h"
#include "columns_ui-sdk/colours.h"
#include "columns_ui-sdk/fonts.h"
#include "columns_ui-sdk/font_utils.h"
#include <foobar2000/SDK/cfg_var.h>
#include <foobar2000/SDK/titleformat.h>

// windowsx.h の SelectBitmap マクロが WTL の SelectBitmap() メンバー関数と
// 衝突するため、無効化する
#ifdef SelectBitmap
#undef SelectBitmap
#endif

#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

// GDI+ の内部実装がテンプレート引数として min/max を使うため、
// windows.h由来のmin/maxマクロが定義されている場合は退避してから
// 無効化しておく（v4.0.0：遠近感演出の高品質補間対応）
#ifdef min
#pragma push_macro("min")
#undef min
#define ALBUMTRAIN_RESTORE_MIN_MACRO
#endif
#ifdef max
#pragma push_macro("max")
#undef max
#define ALBUMTRAIN_RESTORE_MAX_MACRO
#endif

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#ifdef ALBUMTRAIN_RESTORE_MIN_MACRO
#pragma pop_macro("min")
#undef ALBUMTRAIN_RESTORE_MIN_MACRO
#endif
#ifdef ALBUMTRAIN_RESTORE_MAX_MACRO
#pragma pop_macro("max")
#undef ALBUMTRAIN_RESTORE_MAX_MACRO
#endif

namespace {

    // -------------------------------------------------------
    // GDI+ の初期化・終了処理
    // このコンポーネント（DLL）のロード時に構築され、アンロード時に
    // 破棄される静的オブジェクトを使うことで、GdiplusStartup/Shutdown を
    // 確実に対応付ける
    // -------------------------------------------------------
    struct GdiplusInitializer
    {
        ULONG_PTR token = 0;
        GdiplusInitializer()
        {
            Gdiplus::GdiplusStartupInput input;
            Gdiplus::GdiplusStartup(&token, &input, nullptr);
        }
        ~GdiplusInitializer()
        {
            Gdiplus::GdiplusShutdown(token);
        }
    };
    static GdiplusInitializer g_gdiplus_init;

    // Columns UI パネル用の固有GUID
    static const GUID guid_albumtrain_cui =
    { 0x87654321, 0x4321, 0x4321,{0x21,0x43,0x65,0x87,0x09,0xba,0xdc,0xfe} };

    // タイマーID
    static const UINT_PTR TIMER_SCROLL = 1;
    static const UINT_PTR TIMER_ARTWORK = 2;

    // 設定ダイアログのコントロールID
    static const int IDC_SPEED_SLIDER = 1001;
    static const int IDC_SPEED_LABEL = 1002;
    static const int IDC_FORMAT_LABEL = 1003;
    static const int IDC_FORMAT_EDIT = 1004;
    static const int IDC_FONT_BUTTON = 1005;
    static const int IDC_THEME_CHECK = 1006;
    static const int IDC_BG_COLOR_BTN = 1007;
    static const int IDC_TEXT_COLOR_BTN = 1008;
    static const int IDC_NOART_COLOR_BTN = 1009;
    static const int IDC_FILL_BG_CHECK = 1010;
    static const int IDC_FORMAT2_LABEL = 1011;
    static const int IDC_FORMAT2_EDIT = 1012;
    static const int IDC_ADD_LINE_BTN = 1013;
    static const int IDC_GAP1_EDIT = 1014;
    static const int IDC_GAP1_SPIN = 1015;
    static const int IDC_GAP2_EDIT = 1016;
    static const int IDC_GAP2_SPIN = 1017;
    static const int IDC_SHOWNOART_CHECK = 1018;
    static const int IDC_WHEEL_CHECK = 1019;
    static const int IDC_SHOWNAME_CHECK = 1020;
    static const int IDC_PERSPECTIVE_CHECK = 1021;
    static const int IDC_DIR_LTR = 1022;
    static const int IDC_DIR_RTL = 1023;
    static const int IDC_MODE_CUSTOM = 1024;
    static const int IDC_MODE_STABILITY = 1025;
    static const int IDC_APPLY_BTN = 1026;
    static const int IDC_BG_COLOR_SWATCH = 1027;
    static const int IDC_TEXT_COLOR_SWATCH = 1028;
    static const int IDC_NOART_COLOR_SWATCH = 1029;
   
    static const int IDC_BG_MODE_IMAGE = 1031;
    static const int IDC_BG_IMAGE_EDIT = 1032;
    static const int IDC_BG_IMAGE_BROWSE_BTN = 1033;
  
    static const int IDC_NOART_MODE_IMAGE = 1035;
    static const int IDC_NOART_IMAGE_EDIT = 1036;
    static const int IDC_NOART_IMAGE_BROWSE_BTN = 1037;
    static const int IDC_FIXTEXTPOS_CHECK = 1038;
    static const int IDC_ARTWORK_QUALITY_COMBO = 1039;
    static const int IDC_BG_IMAGE_QUALITY_COMBO = 1040;
    static const int IDC_HOVER_FRAME_CHECK = 1041;
    static const int IDC_HOVER_FRAME_COLOR_BTN = 1042;
    static const int IDC_HOVER_FRAME_COLOR_SWATCH = 1043;

    // -------------------------------------------------------
    // 設定項目：スクロール速度（1～10段階、初期値5）
    // -------------------------------------------------------
    static const GUID guid_cfg_scroll_speed =
    { 0x13579bdf, 0x2468, 0xace0,{0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88} };

    static cfg_int g_cfg_scroll_speed(guid_cfg_scroll_speed, 5);

    // -------------------------------------------------------
    // 設定項目：アートワーク未登録曲を表示するか（true=表示する）
    // -------------------------------------------------------
    static const GUID guid_cfg_show_no_artwork =
    { 0x24681357, 0x9bdf, 0xeca0,{0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11} };

    static cfg_bool g_cfg_show_no_artwork(guid_cfg_show_no_artwork, true);

    // -------------------------------------------------------
    // 設定項目：マウスホイールでのスクロールを許可するか（true=許可）
    // -------------------------------------------------------
    static const GUID guid_cfg_allow_wheel_scroll =
    { 0x35792468, 0xace0, 0x1357,{0x9b,0xdf,0x24,0x68,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_allow_wheel_scroll(guid_cfg_allow_wheel_scroll, true);

    // -------------------------------------------------------
    // 設定項目：アルバム名を表示するか（true=表示する）
    // -------------------------------------------------------
    static const GUID guid_cfg_show_album_name =
    { 0x46813579, 0xbdf2, 0x468a,{0xce,0x01,0x35,0x79,0x24,0x68,0xac,0xe0} };

    static cfg_bool g_cfg_show_album_name(guid_cfg_show_album_name, true);

    // -------------------------------------------------------
    // 設定項目：アルバム名表示のTitle Formattingスクリプト
    // -------------------------------------------------------
    static const GUID guid_cfg_album_format =
    { 0x57924680, 0xeca1, 0x3579,{0x68,0xac,0xe0,0x13,0x57,0x9b,0xdf,0x24} };

    static cfg_string g_cfg_album_format(guid_cfg_album_format, "%album%");

    // -------------------------------------------------------
    // 設定項目：アルバム名表示用フォント（LOGFONT構造体を保存）
    // -------------------------------------------------------
    static const GUID guid_cfg_album_font =
    { 0x68ace013, 0x579b, 0xdf24,{0x68,0xac,0xe0,0x13,0x57,0x9b,0xdf,0x24} };

    // 初期値：システムのデフォルトフォント相当
    static LOGFONT make_default_log_font()
    {
        LOGFONT lf = {};
        lf.lfHeight = -12;  // 約9pt相当
        lf.lfWeight = FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wcscpy_s(lf.lfFaceName, L"MS Shell Dlg");
        return lf;
    }

    static cfg_struct_t<LOGFONT> g_cfg_album_font(guid_cfg_album_font, make_default_log_font());

    // -------------------------------------------------------
    // 設定項目：テキスト位置をパネル下方に固定するか
    // （true=最大アートワークサイズ基準の固定位置／false=各エントリのアートワークサイズに追従）
    // -------------------------------------------------------
    static const GUID guid_cfg_fix_text_position =
    { 0x99ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_bool g_cfg_fix_text_position(guid_cfg_fix_text_position, false);

    // -------------------------------------------------------
    // 設定項目：アートワーク表示品質（0=High/1=Middle/2=Low）
    // Stability Focusedモード時は、この値に関わらず強制的にLow(2)として扱う
    // -------------------------------------------------------
    static const GUID guid_cfg_artwork_quality =
    { 0x9aace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_artwork_quality(guid_cfg_artwork_quality, 0);  // デフォルト：High（現状と同じ挙動）

    // -------------------------------------------------------
    // 設定項目：背景画像の表示品質（0=High/1=Middle/2=Low）
    // Artworkグループの品質設定とは独立。Stability Focusedモード時は
    // この値に関わらず強制的にLow(2)として扱う
    // -------------------------------------------------------
    static const GUID guid_cfg_bg_image_quality =
    { 0x9bace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_bg_image_quality(guid_cfg_bg_image_quality, 0);  // デフォルト：High

    // =======================================================
    // ヘルパー関数：2つのLOGFONTが完全に一致するか判定する
    // （ゼロ初期化されたLOGFONTはパディング含め決定的な内容になるため、
    // 単純なバイト比較で問題ない）
    // =======================================================
    static bool IsSameLogFont(const LOGFONT& a, const LOGFONT& b)
    {
        return memcmp(&a, &b, sizeof(LOGFONT)) == 0;
    }

    // =======================================================
    // ヘルパー関数：ダイアログに設定されているフォントの実際の
    // 行の高さ(px)を取得する
    // DS_SHELLFONTでシステムフォントを使っているため、環境によって
    // 数値が変わることを考慮し、実測してレイアウトに反映する
    // =======================================================
    int GetDialogFontLineHeight(HWND hDlg)
    {
        HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
        if (!hFont) return 16;  // フォールバック

        HDC hdc = GetDC(hDlg);
        HFONT hOld = (HFONT)SelectObject(hdc, hFont);

        TEXTMETRIC tm = {};
        GetTextMetrics(hdc, &tm);

        SelectObject(hdc, hOld);
        ReleaseDC(hDlg, hdc);

        int h = tm.tmHeight + tm.tmExternalLeading;
        if (h < 14) h = 14;
        return h;
    }

    // -------------------------------------------------------
    // 設定項目：色のテーマ追従（true=テーマ追従、false=カスタム色を使う）
    // 1つのスイッチで背景色・テキスト色・欠落時の色をまとめて切り替える
    // -------------------------------------------------------
    static const GUID guid_cfg_use_theme_colors =
    { 0x79ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_use_theme_colors(guid_cfg_use_theme_colors, true);

    // -------------------------------------------------------
    // 設定項目：カスタム背景色
    // -------------------------------------------------------
    static const GUID guid_cfg_bg_color =
    { 0x80ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_struct_t<COLORREF> g_cfg_bg_color(guid_cfg_bg_color, RGB(0, 0, 0));

    // -------------------------------------------------------
    // 設定項目：カスタムテキスト色
    // -------------------------------------------------------
    static const GUID guid_cfg_text_color =
    { 0x82ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_struct_t<COLORREF> g_cfg_text_color(guid_cfg_text_color, RGB(255, 255, 255));

    // -------------------------------------------------------
    // 設定項目：カスタムアートワーク欠落時の色
    // -------------------------------------------------------
    static const GUID guid_cfg_noart_color =
    { 0x84ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_struct_t<COLORREF> g_cfg_noart_color(guid_cfg_noart_color, RGB(80, 80, 80));

    // -------------------------------------------------------
    // 設定項目：背景を塗りつぶすか（false=透明にする）
    // -------------------------------------------------------
    static const GUID guid_cfg_fill_background =
    { 0x85ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_fill_background(guid_cfg_fill_background, true);

    // -------------------------------------------------------
    // 設定項目：遠近感演出の有無（true=有効）
    // -------------------------------------------------------
    static const GUID guid_cfg_use_perspective =
    { 0x86ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_use_perspective(guid_cfg_use_perspective, true);

    // -------------------------------------------------------
    // 設定項目：マウスオーバー時にアートワークへ枠を表示するか（true=有効）
    // -------------------------------------------------------
    static const GUID guid_cfg_show_hover_frame =
    { 0x9cace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_show_hover_frame(guid_cfg_show_hover_frame, true);

    // -------------------------------------------------------
    // 設定項目：マウスオーバー枠のカスタム色
    // （Use Theme Coloursが有効な場合はColumns UIの
    //  「Selected item frame」色に追従する）
    // -------------------------------------------------------
    static const GUID guid_cfg_hover_frame_color =
    { 0x9dace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_struct_t<COLORREF> g_cfg_hover_frame_color(guid_cfg_hover_frame_color, RGB(255, 255, 255));

    // -------------------------------------------------------
    // 設定項目：流れる方向（true=右から左、false=左から右）
    // -------------------------------------------------------
    static const GUID guid_cfg_scroll_right_to_left =
    { 0x87ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_scroll_right_to_left(guid_cfg_scroll_right_to_left, true);

    // -------------------------------------------------------
    // 設定項目：2行目表示の有無とTitle Formattingスクリプト
    // -------------------------------------------------------
    static const GUID guid_cfg_use_second_line =
    { 0x88ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static const GUID guid_cfg_album_format2 =
    { 0x89ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };

    static cfg_bool g_cfg_use_second_line(guid_cfg_use_second_line, false);
    static cfg_string g_cfg_album_format2(guid_cfg_album_format2, "");

    // -------------------------------------------------------
    // 設定項目：アートワークとテキストブロックの間隔（1～10px）
    // -------------------------------------------------------
    static const GUID guid_cfg_art_text_gap =
    { 0x90ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_art_text_gap(guid_cfg_art_text_gap, 4);

    // -------------------------------------------------------
    // 設定項目：1行目と2行目の行間（1～10px）
    // -------------------------------------------------------
    static const GUID guid_cfg_line_gap =
    { 0x91ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_line_gap(guid_cfg_line_gap, 2);

    // -------------------------------------------------------
    // 設定項目：設定ダイアログの表示位置（記憶用、内部専用）
    // 未設定（＝初回起動）を示す値としてDIALOG_POS_UNSETを使う
    // -------------------------------------------------------
    static const int DIALOG_POS_UNSET = -999999;

    static const GUID guid_cfg_dialog_pos_x =
    { 0x93ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_dialog_pos_x(guid_cfg_dialog_pos_x, DIALOG_POS_UNSET);

    static const GUID guid_cfg_dialog_pos_y =
    { 0x94ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_int g_cfg_dialog_pos_y(guid_cfg_dialog_pos_y, DIALOG_POS_UNSET);

    // -------------------------------------------------------
    // 設定項目：安定性重視モード（true=軽量・カスタマイズ性重視モード（false）と排他）
    // -------------------------------------------------------
    static const GUID guid_cfg_stability_mode =
    { 0x92ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_bool g_cfg_stability_mode(guid_cfg_stability_mode, true);

    // -------------------------------------------------------
    // 設定項目：背景に任意画像を使うか（true=画像／false=色で塗りつぶし）
    // -------------------------------------------------------
    static const GUID guid_cfg_use_bg_image =
    { 0x95ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_bool g_cfg_use_bg_image(guid_cfg_use_bg_image, false);

    static const GUID guid_cfg_bg_image_path =
    { 0x96ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_string g_cfg_bg_image_path(guid_cfg_bg_image_path, "");

    // -------------------------------------------------------
    // 設定項目：アートワーク欠落時に任意画像を使うか（true=画像／false=色で塗りつぶし）
    // -------------------------------------------------------
    static const GUID guid_cfg_use_noart_image =
    { 0x97ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_bool g_cfg_use_noart_image(guid_cfg_use_noart_image, false);

    static const GUID guid_cfg_noart_image_path =
    { 0x98ace013, 0x68df, 0x2457,{0x9b,0x35,0x7e,0xc0,0x13,0x57,0x9b,0xdf} };
    static cfg_string g_cfg_noart_image_path(guid_cfg_noart_image_path, "");

    // =======================================================
    // ヘルパー関数：WICのフレームをHBITMAPに変換する共通処理
    // （メモリ上のアートワークからのデコード／ファイルからのデコード両方で使う）
    // =======================================================
    HBITMAP DecodeWICFrameToBitmap(IWICImagingFactory* pFactory, IWICBitmapDecoder* pDecoder)
    {
        IWICBitmapFrameDecode* pFrame = nullptr;
        HRESULT hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr)) return NULL;

        IWICFormatConverter* pConverter = nullptr;
        pFactory->CreateFormatConverter(&pConverter);
        pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGR,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

        UINT width = 0, height = 0;
        pConverter->GetSize(&width, &height);

        // v4.0.0：埋め込みアートワークが数千px級の高解像度だと、毎フレームの
        // 拡大縮小描画（特にUltra品質のGDI+高品質補間）が非常に重くなり、
        // 「特定のアルバムだけ極端に重い」「Ultraでほぼフリーズする」の
        // 原因になっていた。そのため、デコードの時点で妥当な上限サイズに
        // 縮小しておく（パネル内での実際の表示サイズより十分大きい値に
        // 設定してあるので、通常利用で画質が劣化することはないはず）
        const UINT kMaxDecodedDim = 512;

        IWICBitmapSource* pFinalSource = pConverter;
        IWICBitmapScaler* pScaler = nullptr;

        UINT longestSide = (width > height) ? width : height;
        if (longestSide > kMaxDecodedDim)
        {
            double ratio = (double)kMaxDecodedDim / (double)longestSide;
            UINT newWidth = (UINT)(width * ratio + 0.5);
            UINT newHeight = (UINT)(height * ratio + 0.5);
            if (newWidth < 1) newWidth = 1;
            if (newHeight < 1) newHeight = 1;

            HRESULT hrScale = pFactory->CreateBitmapScaler(&pScaler);
            if (SUCCEEDED(hrScale))
            {
                hrScale = pScaler->Initialize(pConverter, newWidth, newHeight, WICBitmapInterpolationModeFant);
                if (SUCCEEDED(hrScale))
                {
                    pFinalSource = pScaler;
                    width = newWidth;
                    height = newHeight;
                }
            }
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = (LONG)width;
        bmi.bmiHeader.biHeight = -(LONG)height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HDC hdcScreen = GetDC(NULL);
        HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi,
            DIB_RGB_COLORS, &pBits, NULL, 0);
        ReleaseDC(NULL, hdcScreen);

        if (hBitmap && pBits)
            pFinalSource->CopyPixels(nullptr, width * 4, width * height * 4, (BYTE*)pBits);

        if (pScaler) pScaler->Release();
        pConverter->Release();
        pFrame->Release();
        return hBitmap;
    }

    // =======================================================
    // 診断ログ出力ヘルパー（v3.5.3診断ビルドで導入。グレー表示バグ調査後は
    // 通常利用時にコンソールを埋めてしまわないよう、実際の出力
    // （console::print行）をコメントアウトして無効化してある。
    // 再度トラブルシュートしたくなった際は、この2関数内の
    // console::print(fmt); のコメントを外すだけで再度有効化できる）
    // pfc::string_formatter() のような無名の一時オブジェクトに直接 << を
    // 使うと「非定数の参照は左辺値へのみバインドされない」エラーになるため、
    // 必ず名前付きのローカル変数を経由してから console::print() に渡す
    // =======================================================
    static void DiagLog(const char* label, const char* msg)
    {
        pfc::string_formatter fmt;
        fmt << "[Album Train diag] " << label << ": " << msg;
        // console::print(fmt);  // ← 通常運用では無効化中。再有効化はこの行のコメントを外す
        (void)fmt;  // 未使用警告の抑制
    }

    static void DiagLogNum(const char* label, const char* msg, long long value)
    {
        pfc::string_formatter fmt;
        fmt << "[Album Train diag] " << label << ": " << msg << value;
        // console::print(fmt);  // ← 通常運用では無効化中。再有効化はこの行のコメントを外す
        (void)fmt;  // 未使用警告の抑制
    }

    // =======================================================
    // ヘルパー関数：album_art_data を HBITMAP に変換
    // v3.5.3診断ビルド：label（アルバム名など）を渡すと、WICデコードの
    // どの段階で失敗したかをfoobar2000コンソールへ出力する
    // =======================================================
    HBITMAP DecodeArtToBitmap(album_art_data_ptr art, const char* label = nullptr)
    {
        if (!art.is_valid())
        {
            if (label) DiagLog(label, "art データが無効（is_valid()==false）");
            return NULL;
        }

        if (label)
        {
            DiagLogNum(label, "埋め込み画像データサイズ(bytes) = ", (long long)art->get_size());
        }

        IWICImagingFactory* pFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
        if (FAILED(hr))
        {
            if (label) DiagLogNum(label, "CoCreateInstance(WICImagingFactory) 失敗 HRESULT(10進)=", (long long)hr);
            return NULL;
        }

        IWICStream* pStream = nullptr;
        hr = pFactory->CreateStream(&pStream);
        if (FAILED(hr))
        {
            if (label) DiagLogNum(label, "CreateStream 失敗 HRESULT(10進)=", (long long)hr);
            pFactory->Release(); return NULL;
        }

        hr = pStream->InitializeFromMemory(
            (BYTE*)art->get_ptr(), (DWORD)art->get_size());
        if (FAILED(hr))
        {
            if (label) DiagLogNum(label, "InitializeFromMemory 失敗 HRESULT(10進)=", (long long)hr);
            pStream->Release(); pFactory->Release(); return NULL;
        }

        IWICBitmapDecoder* pDecoder = nullptr;
        hr = pFactory->CreateDecoderFromStream(
            pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder);
        if (FAILED(hr))
        {
            if (label) DiagLogNum(label, "CreateDecoderFromStream 失敗（画像形式を認識できない可能性）HRESULT(10進)=", (long long)hr);
            pStream->Release(); pFactory->Release(); return NULL;
        }

        HBITMAP result = DecodeWICFrameToBitmap(pFactory, pDecoder);

        if (label && !result)
            DiagLog(label, "DecodeWICFrameToBitmap がNULLを返した（フレーム取得・変換段階で失敗）");
        else if (label)
            DiagLog(label, "デコード成功");

        pDecoder->Release();
        pStream->Release();
        pFactory->Release();

        return result;
    }

    // =======================================================
    // ヘルパー関数：ファイルパスから画像を読み込みHBITMAPに変換
    // 背景画像・No-Artwork画像の読み込みに使う
    // =======================================================
    HBITMAP DecodeImageFileToBitmap(const char* utf8_path)
    {
        if (!utf8_path || !*utf8_path) return NULL;

        IWICImagingFactory* pFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
        if (FAILED(hr)) return NULL;

        pfc::stringcvt::string_os_from_utf8 wpath(utf8_path);

        IWICBitmapDecoder* pDecoder = nullptr;
        hr = pFactory->CreateDecoderFromFilename(
            wpath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
        if (FAILED(hr)) { pFactory->Release(); return NULL; }

        HBITMAP result = DecodeWICFrameToBitmap(pFactory, pDecoder);

        pDecoder->Release();
        pFactory->Release();

        return result;
    }

    // =======================================================
    // 指定した矩形内に、アスペクト比を維持したまま画像を中央寄せで描画する
    // （はみ出さない部分は下地のまま＝呼び出し側で事前に塗っておく）
    // =======================================================
    void DrawImageContain(HDC dc, const RECT& rc, HBITMAP bmp, DWORD stretchMode)
    {
        if (!bmp) return;

        BITMAP bm = {};
        GetObject(bmp, sizeof(bm), &bm);
        if (bm.bmWidth <= 0 || bm.bmHeight <= 0) return;

        int rcW = rc.right - rc.left;
        int rcH = rc.bottom - rc.top;
        if (rcW <= 0 || rcH <= 0) return;

        float scale = min((float)rcW / (float)bm.bmWidth, (float)rcH / (float)bm.bmHeight);
        int dstW = (int)(bm.bmWidth * scale + 0.5f);
        int dstH = (int)(bm.bmHeight * scale + 0.5f);
        int dstX = rc.left + (rcW - dstW) / 2;
        int dstY = rc.top + (rcH - dstH) / 2;

        HDC memDC = CreateCompatibleDC(dc);
        HBITMAP hOld = (HBITMAP)SelectObject(memDC, bmp);

        SetStretchBltMode(dc, stretchMode);
        SetBrushOrgEx(dc, 0, 0, NULL);
        StretchBlt(dc, dstX, dstY, dstW, dstH, memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

        SelectObject(memDC, hOld);
        DeleteDC(memDC);
    }
       
    // =======================================================
    // 色プレビュー（スウォッチ）用のサブクラスプロシージャ
    // SS_SUNKENの標準描画がテーマ環境でWM_CTLCOLORSTATICのブラシを
    // 無視してしまうため、WM_PAINTを自前で処理して確実に色を反映する
    // dwRefDataには、表示すべきHBRUSHを保持しているメンバ変数への
    // ポインタ（HBRUSH*）を渡す。値そのものではなくポインタなので、
    // 呼び出し側がブラシを差し替えるだけで自動的に最新の色が反映される
    // =======================================================
    static LRESULT CALLBACK ColorSwatchSubclassProc(
        HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (msg)
        {
        case WM_ERASEBKGND:
            // ちらつき防止：背景消去はWM_PAINT側でまとめて行う
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH* pBrush = (HBRUSH*)dwRefData;
            HBRUSH brush = (pBrush && *pBrush) ? *pBrush : (HBRUSH)GetStockObject(GRAY_BRUSH);

            FillRect(dc, &rc, brush);
            DrawEdge(dc, &rc, EDGE_SUNKEN, BF_RECT);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, ColorSwatchSubclassProc, uIdSubclass);
            break;
        }
        return DefSubclassProc(hwnd, msg, wp, lp);
    }
    
    // =======================================================
    // メインパネルクラス（container_uie_window_v3 ベース）
    // =======================================================
      class AlbumTrainPanel :
        public uie::container_uie_window_v3,
        public play_callback,
        public library_callback_v2_dynamic_impl_base
    {
    public:

        // -------------------------------------------------------
        // キューの1コマ
        // -------------------------------------------------------
        struct QueueEntry {
            pfc::string8      album_name;
            pfc::string8      display_name;   // 1行目の表示文字列
            pfc::string8      display_name2;  // 2行目の表示文字列
            metadb_handle_ptr first_track;
            float             x = 0.0f;
            HBITMAP           hBitmap = NULL;
            bool              art_loaded = false;
        };

        // -------------------------------------------------------
        // ライブラリインデックス（アルバム名・代表トラックに加え、
        // クリック時の全曲再生・Properties表示のため全曲リストも保持する）
        // -------------------------------------------------------
        struct LibAlbum {
            pfc::string8      album_name;
            metadb_handle_ptr first_track;
            std::vector<metadb_handle_ptr> tracks;  // ディスク番号・トラック番号順
        };

        // -------------------------------------------------------
        // アートワークキャッシュの1エントリ
        // ビットマップ本体は参照カウントが0になった時点で解放し、
        // 「有無確認済みか」「実際に存在するか」というメタ情報だけを
        // 保持し続けることで、次回同じアルバムが選ばれた際の
        // 存在チェックを省略できるようにする
        // -------------------------------------------------------
        struct ArtCacheEntry
        {
            bool    checked = false;     // 有無の確認が済んでいるか
            bool    hasArtwork = false;  // アートワークが実際に存在するか
            HBITMAP bitmap = NULL;       // デコード済みビットマップ（参照が無い間はNULL）
            int     refCount = 0;        // 現在キュー内から参照されている数

            // v4.0.0：Artwork QualityがUltraの時のみ使う、GDI+高品質補間
            // 描画用のラッパー。bitmap（HBITMAP）から初回描画時に1回だけ
            // 生成し、bitmapと同じタイミングで解放する（毎フレーム
            // HBITMAPをラップし直すコストを避けるためのキャッシュ）
            Gdiplus::Bitmap* gdipBitmap = nullptr;

            // v4.0.0（ミップマップ的方式）：gdipBitmap（最大512px）を、この
            // パネルで実際に必要な最大表示サイズへあらかじめ1回だけ
            // 高品質に縮小しておいたもの。毎フレームの最終リサイズ倍率を
            // 1:1に近づけることで、連続的な拡縮によるリサンプリングの
            // ちらつき（シマー）を減らす狙い。元画像がそもそも小さい場合は
            // 生成せず、gdipBitmapをそのまま使う
            Gdiplus::Bitmap* gdipBitmapPrescaled = nullptr;
            float            gdipPrescaledForSize = 0.0f;  // 生成時のmax_art_size
        };

        // -------------------------------------------------------
        // 設定ダイアログのステージング領域
        // Apply/OKが押されるまで実際の設定には反映しない
        // -------------------------------------------------------
        struct SettingsStaging
        {
            int      speed = 5;
            bool     showNoArt = true;
            bool     wheelScroll = true;
            bool     showAlbumName = true;
            bool     perspective = true;
            bool     scrollRTL = true;
            bool     useSecondLine = false;
            bool     useThemeColors = true;
            bool     stabilityMode = false;
            int      gap1 = 4;
            int      gap2 = 2;
            LOGFONT  font = make_default_log_font();
            COLORREF bgColor = RGB(0, 0, 0);
            COLORREF textColor = RGB(255, 255, 255);
            COLORREF noArtColor = RGB(80, 80, 80);
            bool     bgUseImage = false;
            pfc::string8 bgImagePath;
            bool     noArtUseImage = false;
            pfc::string8 noArtImagePath;
            int      artworkQuality = 0;  // 0=High/1=Middle/2=Low
            int      bgImageQuality = 0;  // 0=High/1=Middle/2=Low（Backgroundグループ専用）
            bool     fixTextPosition = false;
            bool     showHoverFrame = true;
            COLORREF hoverFrameColor = RGB(255, 255, 255);
        };

        struct SettingsDialogContext
        {
            AlbumTrainPanel* panel = nullptr;
            SettingsStaging  staging;
            HBRUSH hBgBrush = NULL;
            HBRUSH hTextBrush = NULL;
            HBRUSH hNoArtBrush = NULL;
            HBRUSH hHoverFrameBrush = NULL;
        };

        // -------------------------------------------------------
        // コンストラクタ
        // -------------------------------------------------------
        AlbumTrainPanel()
        {
            m_rng.seed((unsigned int)time(NULL));

      
            static_api_ptr_t<play_callback_manager> pcm;
            pcm->register_callback(
                this,
                play_callback::flag_on_playback_new_track |
                play_callback::flag_on_playback_stop,
                false
            );

            // library_callback_v2_dynamic_impl_base のコンストラクタが
            // 自動的にライブラリコールバックを登録してくれる
            // -----------------------------------------------
            // アルバム名フォントの初回デフォルト値を、Columns UIの
            // 「Common (list items)」フォントに合わせる
            // ユーザーが一度でもFontボタンでカスタマイズしていた場合は
            // 何もしない（現在の値をそのまま尊重する）
            // -----------------------------------------------
            {
                LOGFONT currentFont = g_cfg_album_font.get();
                LOGFONT hardcodedDefault = make_default_log_font();

                if (IsSameLogFont(currentFont, hardcodedDefault))
                {
                    try {
                        static_api_ptr_t<cui::fonts::manager_v2> fm;
                        LOGFONT commonFont = fm->get_common_font(cui::fonts::font_type_items);
                        g_cfg_album_font = commonFont;
                    }
                    catch (...) {
                        // Columns UIのフォントマネージャーが取得できない場合は
                        // 何もせず、既存の固定デフォルト値のまま動作させる
                    }
                }
            }
        }

        ~AlbumTrainPanel()
        {
            static_api_ptr_t<play_callback_manager> pcm;
            pcm->unregister_callback(this);
            // library_callback_v2_dynamic_impl_base のデストラクタが
            // 自動的にライブラリコールバックを解除してくれる
            if (m_bgImageBitmap) DeleteObject(m_bgImageBitmap);
            if (m_noArtImageBitmap) DeleteObject(m_noArtImageBitmap);
            ClearAll();
        }

        // -------------------------------------------------------
        // Columns UI のテーマカラーを取得する
        // -------------------------------------------------------
        COLORREF GetThemeColor(cui::colours::colour_identifier_t id) const
        {
            return m_colours.get_colour(id);
        }

        // -------------------------------------------------------
        // 設定に応じて、テーマカラーかカスタムカラーかを切り替えて返す
        // -------------------------------------------------------
        COLORREF GetBackgroundColor() const
        {
            if (g_cfg_use_theme_colors.get())
                return GetThemeColor(cui::colours::colour_background);
            return g_cfg_bg_color.get();
        }

        COLORREF GetTextColor() const
        {
            if (g_cfg_use_theme_colors.get())
                return GetThemeColor(cui::colours::colour_text);
            return g_cfg_text_color.get();
        }

        COLORREF GetNoArtColor() const
        {
            if (g_cfg_use_theme_colors.get())
                return RGB(80, 80, 80);  // 従来の固定グレー
            return g_cfg_noart_color.get();
        }

        COLORREF GetHoverFrameColor() const
        {
            if (g_cfg_use_theme_colors.get())
                return GetThemeColor(cui::colours::colour_active_item_frame);
            return g_cfg_hover_frame_color.get();
        }

        // =======================================================
        // uie::base_window_extension / window が要求する実装
        // =======================================================

        const GUID& get_extension_guid() const override
        {
            return guid_albumtrain_cui;
        }

        void get_name(pfc::string_base& out) const override
        {
            out = "Album Train";
        }

        void get_category(pfc::string_base& out) const override
        {
            out = "Panels";
        }

        unsigned get_type() const override
        {
            return uie::type_panel;
        }

        // -------------------------------------------------------
        // ウィンドウクラスの設定
        // -------------------------------------------------------
        uie::container_window_v3_config get_window_config() override
        {
            return uie::container_window_v3_config(
                L"{ALBUMTRAIN-WINDOW-CUI-V3}",
                true,        // 透明背景を使う（自前で背景を塗るのでtrueでもOK）
                CS_DBLCLKS   // WM_LBUTTONDBLCLKを受け取るために必要
            );
        }

        // =======================================================
        // すべてのウィンドウメッセージをここで処理する
        // =======================================================
        LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) override
        {
            switch (msg)
            {
            case WM_CREATE:
                m_hwnd = wnd;
                BuildLibraryIndex();
                SetTimer(wnd, TIMER_SCROLL, 16, NULL);
                SetTimer(wnd, TIMER_ARTWORK, 100, NULL);
                return 0;

            case WM_DESTROY:
                KillTimer(wnd, TIMER_SCROLL);
                KillTimer(wnd, TIMER_ARTWORK);
                m_hwnd = NULL;
                return 0;

            case WM_SIZE:
                OnSize();
                return 0;

            case WM_TIMER:
                OnTimer(wp);
                return 0;

            case WM_PAINT:
                OnPaint(wnd);
                return 0;

            case WM_LBUTTONDOWN:
                OnLButtonDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;

            case WM_LBUTTONDBLCLK:
                OnLButtonDblClk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;

            case WM_MOUSEMOVE:
                OnMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;

            case WM_MOUSELEAVE:
                m_mouseInWindow = false;
                m_lastHoverEntry = nullptr;
                if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;

            case WM_MOUSEWHEEL:
                OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wp));
                return 0;

            case WM_CONTEXTMENU:
                OnContextMenu(wnd, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;

            case WM_ERASEBKGND:
                // 背景はOnPaintで自前で塗るので、ここでは何もしない
                return 1;
            }

            return DefWindowProc(wnd, msg, wp, lp);
        }

        // =======================================================
        // play_callback の実装
        // =======================================================

        void on_playback_new_track(metadb_handle_ptr track)
        {
            m_current_track = track;
        }

        void on_playback_stop(play_control::t_stop_reason)
        {
            m_current_track.release();
        }

        void on_playback_pause(bool) {}
        void on_playback_starting(play_control::t_track_command, bool) {}
        void on_playback_seek(double) {}
        void on_playback_edited(metadb_handle_ptr) {}
        void on_playback_dynamic_info(const file_info&) {}
        void on_playback_dynamic_info_track(const file_info&) {}
        void on_playback_time(double) {}
        void on_volume_change(float) {}

        // =======================================================
        // library_callback_v2_dynamic の実装
        // ライブラリが起動時に準備完了したらここが呼ばれる
        // =======================================================
        void on_library_initialized() override
        {
            BuildLibraryIndex();
            InitQueue();
            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
        }

    private:

        // =======================================================
        // クライアント領域を取得するヘルパー
        // =======================================================
        RECT GetClientRectHelper() const
        {
            RECT rc = {};
            if (m_hwnd) GetClientRect(m_hwnd, &rc);
            return rc;
        }

        // =======================================================
        // キューエントリが保持しているアートワーク参照を解放する
        // 参照カウントが0になった時点で、実際のビットマップ（ピクセル
        // データ）だけを解放する。「有無確認済み」のメタ情報は
        // キャッシュに残すため、次回同じアルバムが選ばれた際の
        // 存在チェックは省略できる
        // =======================================================
        void ReleaseQueueEntryArt(QueueEntry& e)
        {
            if (!e.hBitmap) return;

            std::string key(e.album_name.c_str());
            auto it = m_art_cache.find(key);
            if (it != m_art_cache.end())
            {
                it->second.refCount--;
                if (it->second.refCount <= 0)
                {
                    it->second.refCount = 0;
                    if (it->second.bitmap)
                    {
                        DeleteObject(it->second.bitmap);
                        it->second.bitmap = NULL;
                    }
                    if (it->second.gdipBitmap)
                    {
                        delete it->second.gdipBitmap;
                        it->second.gdipBitmap = nullptr;
                    }
                    if (it->second.gdipBitmapPrescaled)
                    {
                        delete it->second.gdipBitmapPrescaled;
                        it->second.gdipBitmapPrescaled = nullptr;
                    }
                }
            }
            e.hBitmap = NULL;
        }

        // =======================================================
        // 指定アルバムのHBITMAPに対応するGdiplus::Bitmapを取得する
        // （Artwork QualityがUltraの時のみ使用。初回呼び出し時に1回だけ
        // ラップして m_art_cache 側にキャッシュし、以降のフレームでは
        // 使い回す。hBitmapが変わった場合は呼び出し側の解放処理
        // （ReleaseQueueEntryArt等）で連動して破棄される）
        // =======================================================
        Gdiplus::Bitmap* GetOrCreateGdipBitmap(const pfc::string8& album_name, HBITMAP hBitmap)
        {
            if (!hBitmap) return nullptr;

            std::string key(album_name.c_str());
            auto it = m_art_cache.find(key);
            if (it == m_art_cache.end()) return nullptr;

            if (!it->second.gdipBitmap)
                it->second.gdipBitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);

            return it->second.gdipBitmap;
        }

        // =======================================================
        // v4.0.0（ミップマップ的方式）：GetOrCreateGdipBitmap()で得られる
        // 元画像（最大512px）を、このパネルで実際に必要な最大表示サイズ
        // （targetMaxSize＝max_art_size）へあらかじめ1回だけ高品質に
        // 縮小しておいたものを返す。毎フレームの最終リサイズ倍率を1:1に
        // 近づけることで、連続的な拡縮によるリサンプリングのちらつきを
        // 減らす狙い。元画像がtargetMaxSize以下ならそのまま返す
        // （引き伸ばしはしない）。targetMaxSizeが前回生成時から大きく
        // 変わった場合（パネルリサイズ等）は作り直す
        // =======================================================
        Gdiplus::Bitmap* GetOrCreatePrescaledGdipBitmap(
            const pfc::string8& album_name, HBITMAP hBitmap, float targetMaxSize)
        {
            Gdiplus::Bitmap* base = GetOrCreateGdipBitmap(album_name, hBitmap);
            if (!base || base->GetLastStatus() != Gdiplus::Ok) return base;

            std::string key(album_name.c_str());
            auto it = m_art_cache.find(key);
            if (it == m_art_cache.end()) return base;

            bool needsRebuild = !it->second.gdipBitmapPrescaled ||
                fabs(it->second.gdipPrescaledForSize - targetMaxSize) > 4.0f;

            if (needsRebuild)
            {
                if (it->second.gdipBitmapPrescaled)
                {
                    delete it->second.gdipBitmapPrescaled;
                    it->second.gdipBitmapPrescaled = nullptr;
                }

                UINT srcW = base->GetWidth();
                UINT srcH = base->GetHeight();

                if (srcW > 0 && srcH > 0 && targetMaxSize > 0.0f)
                {
                    UINT longest = (srcW > srcH) ? srcW : srcH;
                    float ratio = targetMaxSize / (float)longest;

                    // 元画像の方が既に小さい場合は、引き伸ばして作り直す
                    // 必要はない（そのままbaseを使う）
                    if (ratio < 1.0f)
                    {
                        UINT newW = (UINT)(srcW * ratio + 0.5f);
                        UINT newH = (UINT)(srcH * ratio + 0.5f);
                        if (newW < 1) newW = 1;
                        if (newH < 1) newH = 1;

                        Gdiplus::Bitmap* prescaled =
                            new Gdiplus::Bitmap(newW, newH, PixelFormat32bppRGB);
                        if (prescaled->GetLastStatus() == Gdiplus::Ok)
                        {
                            Gdiplus::Graphics g(prescaled);
                            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
                            g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
                            g.DrawImage(base, 0.0f, 0.0f, (Gdiplus::REAL)newW, (Gdiplus::REAL)newH);
                            it->second.gdipBitmapPrescaled = prescaled;
                        }
                        else
                        {
                            delete prescaled;
                        }
                    }
                }

                it->second.gdipPrescaledForSize = targetMaxSize;
            }

            return it->second.gdipBitmapPrescaled ? it->second.gdipBitmapPrescaled : base;
        }

        // =======================================================
        // 全データを解放する
        // =======================================================
        void ClearAll()
        {
            for (auto& e : m_queue)
                ReleaseQueueEntryArt(e);
            m_queue.clear();

            // 上記でほぼ全て解放されているはずだが、念のため残っている
            // ビットマップがあれば解放してからキャッシュ自体を破棄する
            for (auto& kv : m_art_cache)
            {
                if (kv.second.bitmap) DeleteObject(kv.second.bitmap);
                if (kv.second.gdipBitmap) delete kv.second.gdipBitmap;
                if (kv.second.gdipBitmapPrescaled) delete kv.second.gdipBitmapPrescaled;
            }
            m_art_cache.clear();

            m_lib_albums.clear();
        }

        // =======================================================
        // メディアライブラリからアルバム一覧を構築（軽量）
        // =======================================================
        void BuildLibraryIndex()
        {
            for (auto& e : m_queue)
                ReleaseQueueEntryArt(e);
            m_queue.clear();
            m_lib_albums.clear();

            static_api_ptr_t<library_manager> lm;
            metadb_handle_list tracks;
            lm->get_all_items(tracks);

            t_size count = tracks.get_count();

            // アルバム名 → 全曲リストを1回の走査で集計する
            // （代表トラックは各アルバムで最初に見つかったトラックとし、
            //   従来の挙動と完全に一致させる）
            std::map<std::string, std::vector<metadb_handle_ptr>> album_map;
            for (t_size i = 0; i < count; i++)
            {
                metadb_handle_ptr track = tracks[i];
                file_info_impl info;
                track->get_info(info);
                const char* album = info.meta_get("ALBUM", 0);
                std::string album_name = album ? album : "Unknown";
                album_map[album_name].push_back(track);
            }

            for (auto& kv : album_map)
            {
                LibAlbum a;
                a.album_name = kv.first.c_str();
                a.first_track = kv.second.front();

                // ディスク番号・トラック番号順にソートしてから保持する。
                // クリック時（AddAlbumToNewPlaylist経由）・Properties表示の
                // どちらも、このソート済みリストをそのまま使い回せる
                metadb_handle_list sorted;
                for (auto& t : kv.second)
                    sorted.add_item(t);
                sorted.sort_by_format("%discnumber%|%tracknumber%", nullptr);

                a.tracks.reserve(sorted.get_count());
                for (t_size i = 0; i < sorted.get_count(); i++)
                    a.tracks.push_back(sorted[i]);

                m_lib_albums.push_back(a);
            }

            // -----------------------------------------------
            // 現在のライブラリに存在しないアルバムのキャッシュエントリを破棄する
            // （直前にキューをクリア済みのため refCount は基本的に0のはずだが、
            //  念のため bitmap が残っていれば解放してから削除する）
            // -----------------------------------------------
            for (auto it = m_art_cache.begin(); it != m_art_cache.end(); )
            {
                if (album_map.find(it->first) == album_map.end())
                {
                    if (it->second.bitmap)
                        DeleteObject(it->second.bitmap);
                    if (it->second.gdipBitmap)
                        delete it->second.gdipBitmap;
                    if (it->second.gdipBitmapPrescaled)
                        delete it->second.gdipBitmapPrescaled;
                    it = m_art_cache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        // =======================================================
        // アートワークサイズを計算するヘルパー
        // =======================================================
        // -------------------------------------------------------
        // テキストブロック全体（アートワークとの間隔＋1行目＋行間＋2行目）の
        // 高さを計算するヘルパー
        // -------------------------------------------------------
        // -------------------------------------------------------
        // 現在の設定フォントの実際の高さ(px)を取得するヘルパー
        // -------------------------------------------------------
        int GetFontLineHeight() const
        {
            LOGFONT lf = g_cfg_album_font.get();
            HFONT hFont = CreateFontIndirect(&lf);
            if (!hFont) return 16;  // フォールバック

            HDC hdc = GetDC(NULL);
            HFONT hOld = (HFONT)SelectObject(hdc, hFont);

            TEXTMETRIC tm = {};
            GetTextMetrics(hdc, &tm);

            SelectObject(hdc, hOld);
            ReleaseDC(NULL, hdc);
            DeleteObject(hFont);

            int h = tm.tmHeight;
            if (h < 10) h = 10;
            return h;
        }

        int GetTextBlockHeight() const
        {
            if (!g_cfg_show_album_name.get()) return 0;

            int lineHeight = GetFontLineHeight();
            int gap = (int)g_cfg_art_text_gap.get();
            int height = gap + lineHeight;

            if (g_cfg_use_second_line.get())
                height += (int)g_cfg_line_gap.get() + lineHeight;

            return height;
        }

        int GetArtSize(const RECT& rc) const
        {
            const float max_scale = 1.2f;
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // テキストブロックの実際の高さを引いた残りをアートワーク用に使う
            // ウィンドウ上下の余白として10pxを確保
            int textBlockHeight = GetTextBlockHeight();
            int availableHeight = height - 10 - textBlockHeight;

            int s = min(width, availableHeight) - 20;
            s = (int)(s / max_scale);
            return s < 10 ? 10 : s;
        }

        // =======================================================
        // アルバム同士の間隔を計算するヘルパー
        // =======================================================
        int GetSpacing(const RECT& rc) const
        {
            const float max_scale = 1.2f;
            int base_art_size = GetArtSize(rc);
            int max_art_size = (int)(base_art_size * max_scale);
            return max_art_size + 20;
        }

        // =======================================================
        // Title Formattingスクリプトをコンパイルする
        // スクリプト文字列が変わっていなければ再コンパイルしない
        // =======================================================
        void EnsureCompiledScript(
            titleformat_object::ptr& compiled_script,
            pfc::string8& cached_format,
            const char* script_text,
            const char* fallback)
        {
            if (compiled_script.is_valid() && cached_format == script_text)
                return;  // 変更なし：再コンパイル不要

            try {
                titleformat_object::ptr script;
                titleformat_compiler::get()->compile_safe_ex(
                    script, script_text, fallback);
                compiled_script = script;
                cached_format = script_text;
            }
            catch (...) {
                compiled_script.release();
                cached_format = script_text;
            }
        }

        // =======================================================
        // コンパイル済みスクリプトを使って1行の表示文字列を生成する
        // 改行はスペースに変換し、UTF-8マルチバイト文字を壊さないようにする
        // =======================================================
        static pfc::string8 ApplyCompiledScript(
            metadb_handle_ptr track, titleformat_object::ptr compiled_script, const char* fallback)
        {
            pfc::string8 result;
            if (!track.is_valid()) return result;

            if (!compiled_script.is_valid())
            {
                result = fallback;
                return result;
            }

            try {
                pfc::string8 formatted;
                track->format_title(NULL, formatted, compiled_script, NULL);

                pfc::string8 singleLine;
                const char* p = formatted.get_ptr();
                const char* segStart = p;
                while (*p)
                {
                    if (*p == '\r' || *p == '\n')
                    {
                        if (p > segStart)
                            singleLine.add_string(segStart, p - segStart);
                        if (singleLine.get_length() > 0 &&
                            singleLine.get_ptr()[singleLine.get_length() - 1] != ' ')
                            singleLine.add_string(" ", 1);

                        while (*p == '\r' || *p == '\n') p++;
                        segStart = p;
                    }
                    else
                    {
                        p++;
                    }
                }
                if (p > segStart)
                    singleLine.add_string(segStart, p - segStart);

                result = singleLine;
            }
            catch (...) {
                result = fallback;
            }

            return result;
        }

        // =======================================================
        // ランダムに1件のQueueEntryを作る
        // =======================================================
        QueueEntry MakeRandomEntry(float x)
        {
            QueueEntry e;
            e.x = x;
            e.art_loaded = false;
            e.hBitmap = NULL;

            if (m_lib_albums.empty()) return e;

            // 「未登録曲を表示しない」設定の場合、キャッシュ済みで
            // アートワークが無いと分かっているアルバムは選び直す
            // （最大20回までリトライ、見つからなければそのまま諦める）
            int idx = 0;
            for (int attempt = 0; attempt < 20; attempt++)
            {
                std::uniform_int_distribution<int> dist(0, (int)m_lib_albums.size() - 1);
                idx = dist(m_rng);
                if (idx == m_last_album_index && (int)m_lib_albums.size() > 1)
                    idx = dist(m_rng);

                if (g_cfg_stability_mode.get() || g_cfg_show_no_artwork.get())
                    break;  // 安定性重視モード、または表示する設定なら何でも良い

                std::string checkKey(m_lib_albums[idx].album_name.c_str());
                auto checkIt = m_art_cache.find(checkKey);

                if (checkIt != m_art_cache.end() && checkIt->second.checked)
                {
                    // 有無確認済み：無いと分かっていれば選び直す、あれば採用
                    if (!checkIt->second.hasArtwork) continue;
                    break;
                }

                // 未確認の場合、ここで軽くアートワークの有無だけ確認する
                // （実際のデコードはまだしない）
                bool hasArtwork = false;
                try {
                    static_api_ptr_t<album_art_manager_v2> aam;
                    pfc::list_single_ref_t<GUID> guids(album_art_ids::cover_front);
                    abort_callback_dummy abort;
                    auto extractor = aam->open(
                        pfc::list_single_ref_t<metadb_handle_ptr>(
                            m_lib_albums[idx].first_track),
                        guids, abort);
                    album_art_data_ptr data;
                    hasArtwork = extractor->query(album_art_ids::cover_front, data, abort);
                }
                catch (const pfc::exception& ex) {
                    pfc::string_formatter what_msg;
                    what_msg << "有無確認パスで例外（pfc::exception）: " << ex.what();
                    DiagLog(m_lib_albums[idx].album_name.c_str(), what_msg.c_str());
                }
                catch (...) {
                    DiagLog(m_lib_albums[idx].album_name.c_str(), "有無確認パスで不明な例外");
                }

                // 有無確認済みの結果として記録しておく（次回以降は即座に判定できる）
                ArtCacheEntry& checkEntry = m_art_cache[checkKey];
                checkEntry.checked = true;
                checkEntry.hasArtwork = hasArtwork;

                if (hasArtwork) break;
            }
            m_last_album_index = idx;

            e.album_name = m_lib_albums[idx].album_name;
            e.first_track = m_lib_albums[idx].first_track;

            // 1行目を生成する（スクリプトは必要な時だけ再コンパイルされる）
            EnsureCompiledScript(m_compiled_script1, m_cached_format1,
                g_cfg_album_format.get().c_str(), "%album%");
            e.display_name = ApplyCompiledScript(
                e.first_track, m_compiled_script1, "%album%");

            // 2行目が有効な場合のみ生成する
            if (g_cfg_use_second_line.get())
            {
                EnsureCompiledScript(m_compiled_script2, m_cached_format2,
                    g_cfg_album_format2.get().c_str(), "");
                e.display_name2 = ApplyCompiledScript(
                    e.first_track, m_compiled_script2, "");
            }

            std::string key(e.album_name.c_str());
            auto it = m_art_cache.find(key);

            if (it != m_art_cache.end() && it->second.checked)
            {
                if (!it->second.hasArtwork)
                {
                    // 有無確認済み・アートワーク無し
                    e.hBitmap = NULL;
                    e.art_loaded = true;
                }
                else if (it->second.bitmap)
                {
                    // 有無確認済み・既にデコード済みのビットマップが
                    // メモリ上に残っている → 参照カウントを増やして共有する
                    it->second.refCount++;
                    e.hBitmap = it->second.bitmap;
                    e.art_loaded = true;
                }
                else
                {
                    // 有無確認済み・存在はするが参照が無くなり解放済み
                    // → FetchOneArtworkで再デコードさせる
                    e.hBitmap = NULL;
                    e.art_loaded = false;
                }
            }
            else
            {
                // 未確認 → FetchOneArtworkで確認・デコードの両方を行わせる
                e.hBitmap = NULL;
                e.art_loaded = false;
            }

            return e;
        }

        // =======================================================
        // キューの初期化：画面を埋める分だけ積む
        // =======================================================
        void InitQueue()
        {
            if (m_lib_albums.empty()) return;

            RECT rc = GetClientRectHelper();
            int width = rc.right - rc.left;

            if (width < 100 || (rc.bottom - rc.top) < 50) return;

            for (auto& e : m_queue)
                ReleaseQueueEntryArt(e);
            m_queue.clear();

            int spacing = GetSpacing(rc);

            int needed = (width / spacing + 4) * 3;
            float start_x = (float)(-spacing * (needed / 3));

            for (int i = 0; i < needed; i++)
                m_queue.push_back(MakeRandomEntry(start_x + i * spacing));
        }

        // =======================================================
        // キューの前後を補充・不要分を削除
        // =======================================================
        void ExtendQueue(int spacing, int screen_width)
        {
            if (m_lib_albums.empty()) return;

            while (m_queue.empty() ||
                m_queue.back().x < (float)(screen_width + spacing * 6))
            {
                float x = m_queue.empty()
                    ? (float)screen_width
                    : m_queue.back().x + spacing;
                m_queue.push_back(MakeRandomEntry(x));
            }

            while (m_queue.empty() ||
                m_queue.front().x > (float)(-spacing * 6))
            {
                float x = m_queue.empty()
                    ? 0.0f
                    : m_queue.front().x - spacing;
                m_queue.push_front(MakeRandomEntry(x));
            }

            while (m_queue.size() > 200 &&
                m_queue.back().x > (float)(screen_width + spacing * 30))
            {
                ReleaseQueueEntryArt(m_queue.back());
                m_queue.pop_back();
            }

            while (m_queue.size() > 200 &&
                m_queue.front().x < (float)(-spacing * 30))
            {
                ReleaseQueueEntryArt(m_queue.front());
                m_queue.pop_front();
            }
        }

        // =======================================================
        // アートワーク取得（1回に1件だけ）
        // =======================================================
        void FetchOneArtwork()
        {
            if (m_queue.empty()) return;

            for (auto& e : m_queue)
            {
                if (e.art_loaded) continue;
                if (!e.first_track.is_valid()) { e.art_loaded = true; continue; }

                std::string key(e.album_name.c_str());
                auto it = m_art_cache.find(key);

                // 既に有無確認済みで、なおかつビットマップも残っている場合は共有する
                if (it != m_art_cache.end() && it->second.checked)
                {
                    if (!it->second.hasArtwork)
                    {
                        e.hBitmap = NULL;
                        e.art_loaded = true;
                        return;
                    }
                    if (it->second.bitmap)
                    {
                        it->second.refCount++;
                        e.hBitmap = it->second.bitmap;
                        e.art_loaded = true;
                        return;
                    }
                    // ここに来るのは「存在は確認済みだが、参照が無くなり
                    // 解放済み」のケース。この下のデコード処理へ進む。
                }

                HBITMAP hBitmap = NULL;
                bool hasArtwork = false;
                try {
                    static_api_ptr_t<album_art_manager_v2> aam;
                    pfc::list_single_ref_t<GUID> guids(album_art_ids::cover_front);
                    abort_callback_dummy abort;
                    auto extractor = aam->open(
                        pfc::list_single_ref_t<metadb_handle_ptr>(e.first_track),
                        guids, abort);
                    album_art_data_ptr data;
                    hasArtwork = extractor->query(album_art_ids::cover_front, data, abort);
                    if (hasArtwork)
                        hBitmap = DecodeArtToBitmap(data, e.album_name.c_str());
                }
                catch (const pfc::exception& ex) {
                    pfc::string_formatter what_msg;
                    what_msg << "デコードパスで例外（pfc::exception）: " << ex.what();
                    DiagLog(e.album_name.c_str(), what_msg.c_str());
                }
                catch (...) {
                    DiagLog(e.album_name.c_str(), "デコードパスで不明な例外");
                }

                ArtCacheEntry& entry = m_art_cache[key];
                entry.checked = true;
                entry.hasArtwork = hasArtwork;

                if (hasArtwork && hBitmap)
                {
                    entry.bitmap = hBitmap;
                    entry.refCount++;
                    e.hBitmap = hBitmap;
                }
                else
                {
                    e.hBitmap = NULL;
                }
                e.art_loaded = true;
                return;
            }
        }

        // =======================================================
        // ウィンドウサイズが確定したタイミングでキューを初期化
        // =======================================================
        void OnSize()
        {
            if (m_lib_albums.empty()) return;

            RECT rc = GetClientRectHelper();
            if ((rc.right - rc.left) <= 0) return;

            if (m_queue.empty())
                InitQueue();
        }

        // =======================================================
        // スクロール位置の更新（SetTimer(TIMER_SCROLL)から呼ばれる。
        // v4.0.0でtimeSetEvent方式を試したが、メッセージキューの
        // 詰まりによるアプリ全体の遅延を招いたため、SetTimerに戻した）
        // =======================================================
        void OnScrollTick()
        {
            if (!m_hwnd) return;
            if (m_lib_albums.empty() || m_queue.empty()) return;

            RECT rc = GetClientRectHelper();
            int width = rc.right - rc.left;
            if (width <= 0) return;

            int spacing = GetSpacing(rc);

            // 設定値（1～10）を実際のスクロール速度に変換
            // 従来は30ms間隔で1段階あたり0.4px/フレームだったが、
            // v4.0.0でタイマー間隔を16msに高頻度化したため、
            // 体感速度（px/秒）を変えないよう比例して縮小する
            float speed = (float)g_cfg_scroll_speed.get() * 0.4f * (16.0f / 30.0f);
            if (!g_cfg_scroll_right_to_left.get()) speed = -speed;

            for (auto& entry : m_queue)
                entry.x -= speed;

            ExtendQueue(spacing, width);

            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
        }

        // =======================================================
        // タイマー処理
        // =======================================================
        void OnTimer(UINT_PTR id)
        {
            if (id == TIMER_SCROLL)
            {
                OnScrollTick();
            }
            else if (id == TIMER_ARTWORK)
            {
                FetchOneArtwork();
            }
        }

        // =======================================================
        // 描画処理
        // =======================================================
        void OnPaint(HWND wnd)
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(wnd, &ps);

            RECT rc = GetClientRectHelper();
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            HDC bufDC = CreateCompatibleDC(dc);
            HBITMAP bufBitmap = CreateCompatibleBitmap(dc, width, height);
            HBITMAP hOldBuf = (HBITMAP)SelectObject(bufDC, bufBitmap);

            RECT fillRc = { 0, 0, width, height };

            // アートワーク表示品質（Stability Focusedモードでは強制的にLowにする）
            // v4.0.0：Ultra（内部値3）はGDI+経路を使うが、万一取得に失敗した際の
            // フォールバック用StretchBltモードとしてはHighと同じHALFTONEを使う
            int artworkQuality = g_cfg_stability_mode.get() ? 2 : (int)g_cfg_artwork_quality.get();
            DWORD artworkStretchMode = (artworkQuality == 0 || artworkQuality == 3) ? HALFTONE
                : (artworkQuality == 1) ? COLORONCOLOR
                : STRETCH_DELETESCANS;

            // 背景画像専用の表示品質（Artworkの品質設定とは独立。Stability Focusedモードでは強制Low）
            int bgImageQuality = g_cfg_stability_mode.get() ? 2 : (int)g_cfg_bg_image_quality.get();
            DWORD bgImageStretchMode = (bgImageQuality == 0) ? HALFTONE
                : (bgImageQuality == 1) ? COLORONCOLOR
                : STRETCH_DELETESCANS;

            if (g_cfg_fill_background.get())
            {
                HBRUSH bgBrush = CreateSolidBrush(GetBackgroundColor());
                FillRect(bufDC, &fillRc, bgBrush);
                DeleteObject(bgBrush);

                if (g_cfg_use_bg_image.get() && g_cfg_bg_image_path.get_length() > 0)
                {
                    HBITMAP bgImg = GetOrLoadBgImage();
                    DrawImageContain(bufDC, fillRc, bgImg, bgImageStretchMode);
                }
            }

            // 設定されたフォント（LOGFONT）からHFONTを作成する
            LOGFONT lf = g_cfg_album_font.get();
            HFONT currentFont = CreateFontIndirect(&lf);
            HFONT hOldFont = NULL;
            if (currentFont)
                hOldFont = (HFONT)SelectObject(bufDC, currentFont);

            if (m_queue.empty() && !m_lib_albums.empty() && width > 0)
                InitQueue();

            if (m_lib_albums.empty() || m_queue.empty())
            {
                RECT textRc = fillRc;
                textRc.top = height / 2 - 10;
                SetTextColor(bufDC, GetTextColor());
                SetBkMode(bufDC, TRANSPARENT);
                DrawText(bufDC, _T("Now Loading..."), -1, &textRc, DT_CENTER);

                if (hOldFont)
                    SelectObject(bufDC, hOldFont);
                if (currentFont)
                    DeleteObject(currentFont);

                BitBlt(dc, 0, 0, width, height, bufDC, 0, 0, SRCCOPY);
                SelectObject(bufDC, hOldBuf);
                DeleteObject(bufBitmap);
                DeleteDC(bufDC);
                EndPaint(wnd, &ps);
                return;
            }

            float base_art_size = (float)GetArtSize(rc);
            const float max_scale = 1.2f;
            float max_art_size = base_art_size * max_scale;

            float textBlockHeight = (float)GetTextBlockHeight();
            float content_height = max_art_size + textBlockHeight;

            // ウィンドウ縦中央に収まるよう draw_y を計算する
            float draw_y = ((float)height - content_height) / 2.0f;
            if (draw_y < 5.0f) draw_y = 5.0f;  // 最低5pxの上余白を確保

            // テキスト位置固定オプション用：最大アートワークサイズ基準の固定Y座標を1回だけ計算
            float fixedTextTopY = draw_y + max_art_size + (float)g_cfg_art_text_gap.get();

            float center_x = (float)width / 2.0f;
            float max_dist = (float)width / 2.0f;

            // マウスオーバー枠表示のため、事前に「どのエントリの上に
            // マウスがあるか」をFindHoverEntry()で判定しておく
            QueueEntry* hoverEntry = nullptr;
            if (g_cfg_show_hover_frame.get() && m_mouseInWindow)
                hoverEntry = FindHoverEntry(m_mouseX, m_mouseY);

            // v4.0.0：Artwork QualityがUltraの場合、Gdiplus::Graphicsを
            // エントリごとに毎回作り直すと負荷が積み重なるため、
            // このフレーム内で1回だけ生成して使い回す
            // （Graphics(HDC)は内部にバッファを持たないラッパーのため、
            // 同じbufDC上で他の通常のGDI描画と混在させても問題ない）
            Gdiplus::Graphics* gdipGraphics = nullptr;
            if (artworkQuality == 3)
            {
                gdipGraphics = new Gdiplus::Graphics(bufDC);
                gdipGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                gdipGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
                gdipGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
                gdipGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
            }

            for (auto& entry : m_queue)
            {
                float base_draw_x = entry.x;
                if (base_draw_x + base_art_size < 0 || base_draw_x > width) continue;

                float scale = 1.0f;
                if (g_cfg_use_perspective.get())
                {
                    float entry_center = base_draw_x + base_art_size / 2.0f;
                    float dist = fabs(entry_center - center_x);
                    float t = dist / max_dist;
                    if (t > 1.0f) t = 1.0f;
                    scale = 1.2f - t * 0.2f;
                }

                float art_size = base_art_size * scale;
                if (art_size < 10.0f) art_size = 10.0f;

                // v4.0.0：2px単位への丸めが「段階的な拡大縮小」に見える主因だった
                // ため、Ultra品質の実描画だけはこの丸めをかけない連続値を使う。
                // クリック判定・ホバー枠など見た目以外に関わる箇所は、従来通り
                // 丸めた値（art_size）で統一し、挙動に影響を与えないようにする
                float art_size_rounded = (float)((int)(art_size / 2.0f + 0.5f) * 2);
                float art_size_for_draw = (artworkQuality == 3) ? art_size : art_size_rounded;
                art_size = art_size_rounded;

                float draw_x = base_draw_x + (base_art_size - art_size) / 2.0f;
                float draw_y_adj = draw_y + (max_art_size - art_size) / 2.0f;

                float draw_x_for_draw = base_draw_x + (base_art_size - art_size_for_draw) / 2.0f;
                float draw_y_adj_for_draw = draw_y + (max_art_size - art_size_for_draw) / 2.0f;

                if (entry.hBitmap)
                {
                    BITMAP bm = {};
                    GetObject(entry.hBitmap, sizeof(bm), &bm);

                    if (bm.bmWidth <= 0 || bm.bmHeight <= 0)
                    {
                        continue;
                    }

                    float dst_w, dst_h;
                    if (bm.bmWidth >= bm.bmHeight)
                    {
                        dst_w = art_size_for_draw;
                        dst_h = art_size_for_draw * (float)bm.bmHeight / (float)bm.bmWidth;
                    }
                    else
                    {
                        dst_h = art_size_for_draw;
                        dst_w = art_size_for_draw * (float)bm.bmWidth / (float)bm.bmHeight;
                    }

                    float dst_x = draw_x_for_draw + (art_size_for_draw - dst_w) / 2.0f;
                    float dst_y = draw_y_adj_for_draw + (art_size_for_draw - dst_h) / 2.0f;

                    // v4.0.0：Artwork QualityがUltraの時のみ、GDI+の高品質補間
                    // （HighQualityBicubic）で描画する。遠近感演出中の連続的な
                    // サイズ変化でも輪郭がガクつかず滑らかに見えるようにするため。
                    // High/Middle/Lowは従来通りGDI（StretchBlt）のまま
                    bool drawnWithGdiplus = false;
                    if (artworkQuality == 3 && gdipGraphics)
                    {
                        // v4.0.0（ミップマップ的方式）：このパネルで実際に必要な
                        // 最大表示サイズ（max_art_size）へあらかじめ縮小済みの
                        // ものを使うことで、毎フレームの最終リサイズ倍率を
                        // 1:1に近づける。ソース矩形は、そのビットマップ自身の
                        // 実際の寸法（元画像のbm.bmWidth/bmHeightとは異なる
                        // 場合がある）を使う
                        Gdiplus::Bitmap* gdipBmp = GetOrCreatePrescaledGdipBitmap(
                            entry.album_name, entry.hBitmap, max_art_size);
                        if (gdipBmp && gdipBmp->GetLastStatus() == Gdiplus::Ok)
                        {
                            gdipGraphics->DrawImage(gdipBmp,
                                Gdiplus::RectF(dst_x, dst_y, dst_w, dst_h),
                                0.0f, 0.0f,
                                (Gdiplus::REAL)gdipBmp->GetWidth(), (Gdiplus::REAL)gdipBmp->GetHeight(),
                                Gdiplus::UnitPixel);
                            drawnWithGdiplus = true;
                        }
                    }

                    if (!drawnWithGdiplus)
                    {
                        // 従来のGDI経路（Middle/Low、またはGDI+側の取得に失敗した場合のフォールバック）
                        HDC memDC = CreateCompatibleDC(bufDC);
                        HBITMAP hOld = (HBITMAP)SelectObject(memDC, entry.hBitmap);

                        SetStretchBltMode(bufDC, artworkStretchMode);
                        SetBrushOrgEx(bufDC, 0, 0, NULL);

                        StretchBlt(bufDC,
                            (int)(dst_x + 0.5f), (int)(dst_y + 0.5f),
                            (int)(dst_w + 0.5f), (int)(dst_h + 0.5f),
                            memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

                        SelectObject(memDC, hOld);
                        DeleteDC(memDC);
                    }
                }
                else
                {
                    RECT noArtRc = {
                        (int)(draw_x + 0.5f), (int)(draw_y_adj + 0.5f),
                        (int)(draw_x + art_size + 0.5f), (int)(draw_y_adj + art_size + 0.5f)
                    };

                    HBITMAP noArtImg = (g_cfg_use_noart_image.get() && g_cfg_noart_image_path.get_length() > 0)
                        ? GetOrLoadNoArtImage() : NULL;

                    if (noArtImg)
                    {
                        DrawImageContain(bufDC, noArtRc, noArtImg, artworkStretchMode);
                    }
                    else
                    {
                        HBRUSH grayBrush = CreateSolidBrush(GetNoArtColor());
                        FillRect(bufDC, &noArtRc, grayBrush);
                        DeleteObject(grayBrush);
                    }
                }

                // マウスオーバー枠：アートワークのみを縁取る（固定2px）
                // v4.0.0：Ultra品質の時は、アートワーク本体と同じ連続値
                // （draw_x_for_draw / art_size_for_draw）を使う。丸めた値の
                // ままだと、枠だけが2pxずつ段階的に動き、滑らかに動く
                // アートワーク本体との間にズレが生じ、「枠をはみ出すように
                // 振動する」ように見えてしまうため
                if (hoverEntry == &entry)
                {
                    float frame_x = (artworkQuality == 3) ? draw_x_for_draw : draw_x;
                    float frame_y = (artworkQuality == 3) ? draw_y_adj_for_draw : draw_y_adj;
                    float frame_size = (artworkQuality == 3) ? art_size_for_draw : art_size;

                    RECT frameRc = {
                        (int)(frame_x + 0.5f), (int)(frame_y + 0.5f),
                        (int)(frame_x + frame_size + 0.5f), (int)(frame_y + frame_size + 0.5f)
                    };

                    HPEN framePen = CreatePen(PS_SOLID, 2, GetHoverFrameColor());
                    HPEN oldPen = (HPEN)SelectObject(bufDC, framePen);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(bufDC, GetStockObject(NULL_BRUSH));

                    Rectangle(bufDC, frameRc.left, frameRc.top, frameRc.right, frameRc.bottom);

                    SelectObject(bufDC, oldBrush);
                    SelectObject(bufDC, oldPen);
                    DeleteObject(framePen);
                }

                if (g_cfg_show_album_name.get())
                {
                    float lineHeight = (float)GetFontLineHeight();
                    float gap1 = (float)g_cfg_art_text_gap.get();

                    int textLeft = (int)(draw_x + 0.5f);
                    int textWidth = (int)(art_size + 0.5f);

                    float textTopY = g_cfg_fix_text_position.get()
                        ? fixedTextTopY
                        : (draw_y_adj + art_size + gap1);

                    RECT nameRc = {
                        textLeft,
                        (int)(textTopY + 0.5f),
                        textLeft + textWidth,
                        (int)(textTopY + lineHeight + 0.5f)
                    };
                    SetTextColor(bufDC, GetTextColor());
                    SetBkMode(bufDC, TRANSPARENT);
                    pfc::stringcvt::string_os_from_utf8 name_t(entry.display_name);
                    DrawText(bufDC, name_t, -1, &nameRc, DT_CENTER | DT_END_ELLIPSIS);

                    // 2行目が有効な場合は、その下にもう1行描画する
                    if (g_cfg_use_second_line.get())
                    {
                        float gap2 = (float)g_cfg_line_gap.get();
                        float line2TopY = textTopY + lineHeight + gap2;

                        RECT nameRc2 = {
                            textLeft,
                            (int)(line2TopY + 0.5f),
                            textLeft + textWidth,
                            (int)(line2TopY + lineHeight + 0.5f)
                        };
                        pfc::stringcvt::string_os_from_utf8 name2_t(entry.display_name2);
                        DrawText(bufDC, name2_t, -1, &nameRc2, DT_CENTER | DT_END_ELLIPSIS);
                    }
                }
            }

            if (gdipGraphics)
                delete gdipGraphics;

           if (hOldFont)
                SelectObject(bufDC, hOldFont);
            if (currentFont)
                DeleteObject(currentFont);

            BitBlt(dc, 0, 0, width, height, bufDC, 0, 0, SRCCOPY);
            SelectObject(bufDC, hOldBuf);
            DeleteObject(bufBitmap);
            DeleteDC(bufDC);

            EndPaint(wnd, &ps);
        }

        // =======================================================
        // contextmenu_managerが返すメニューツリーを再帰的にたどり、
        // 指定した名前（部分一致）を持つコマンドノードを探す
        // =======================================================
        contextmenu_node* FindMenuNodeByName(contextmenu_node* node, const char* name)
        {
            if (!node) return nullptr;

            if (node->get_type() == contextmenu_item_node::type_command)
            {
                const char* n = node->get_name();
                if (n && strstr(n, name) != nullptr)
                    return node;
                return nullptr;
            }
            else if (node->get_type() == contextmenu_item_node::type_group)
            {
                t_size count = node->get_num_children();
                for (t_size i = 0; i < count; i++)
                {
                    contextmenu_node* found = FindMenuNodeByName(node->get_child(i), name);
                    if (found) return found;
                }
            }
            return nullptr;
        }

        // =======================================================
        // 指定されたクライアント座標にあるアルバムを判定する
        // OnPaintと同じレイアウト計算を用いて、実際の描画位置・サイズで判定する
        // 重なりがある場合は手前（描画順で後）のエントリを優先する
        // =======================================================
        bool HitTestAlbum(int x, int y, pfc::string8& out_album_name, metadb_handle_ptr& out_track)
        {
            if (m_queue.empty()) return false;

            RECT rc = GetClientRectHelper();
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            float base_art_size = (float)GetArtSize(rc);
            const float max_scale = 1.2f;
            float max_art_size = base_art_size * max_scale;

            float textBlockHeight = (float)GetTextBlockHeight();
            float content_height = max_art_size + textBlockHeight;

            float draw_y = ((float)height - content_height) / 2.0f;
            if (draw_y < 5.0f) draw_y = 5.0f;

            // テキスト位置固定オプション用：OnPaintと同じ計算式で固定Y座標を1回だけ求める
            float fixedTextTopY = draw_y + max_art_size + (float)g_cfg_art_text_gap.get();

            float center_x = (float)width / 2.0f;
            float max_dist = (float)width / 2.0f;

            for (auto it = m_queue.rbegin(); it != m_queue.rend(); ++it)
            {
                auto& entry = *it;
                float base_draw_x = entry.x;
                if (base_draw_x + base_art_size < 0 || base_draw_x > width) continue;

                float scale = 1.0f;
                if (g_cfg_use_perspective.get())
                {
                    float entry_center = base_draw_x + base_art_size / 2.0f;
                    float dist = fabs(entry_center - center_x);
                    float t = dist / max_dist;
                    if (t > 1.0f) t = 1.0f;
                    scale = 1.2f - t * 0.2f;
                }

                float art_size = base_art_size * scale;
                if (art_size < 10.0f) art_size = 10.0f;
                art_size = (float)((int)(art_size / 2.0f + 0.5f) * 2);

                float draw_x = base_draw_x + (base_art_size - art_size) / 2.0f;
                float draw_y_adj = draw_y + (max_art_size - art_size) / 2.0f;

                // ヒット領域の下端：テキスト表示が有効な場合は、OnPaintと全く同じ
                // 計算式で求めた「表示中の最後のテキスト行の下端」までを、
                // アートワークとの間の Artwork-Text Gap も含めて1つの連続した
                // 矩形とみなす。テキスト非表示時は従来通りアートワーク下端まで。
                float hitBottom = draw_y_adj + art_size;
                if (g_cfg_show_album_name.get())
                {
                    float lineHeight = (float)GetFontLineHeight();
                    float gap1 = (float)g_cfg_art_text_gap.get();
                    float textTopY = g_cfg_fix_text_position.get()
                        ? fixedTextTopY
                        : (draw_y_adj + art_size + gap1);

                    hitBottom = textTopY + lineHeight;

                    if (g_cfg_use_second_line.get())
                    {
                        float gap2 = (float)g_cfg_line_gap.get();
                        hitBottom = textTopY + lineHeight + gap2 + lineHeight;
                    }
                }

                if (x >= draw_x && x <= draw_x + art_size &&
                    y >= draw_y_adj && y <= hitBottom)
                {
                    if (!entry.first_track.is_valid()) return false;
                    out_album_name = entry.album_name;
                    out_track = entry.first_track;
                    return true;
                }
            }
            return false;
        }

        // =======================================================
        // アルバム名から m_lib_albums 内の該当エントリを探す
        // m_lib_albums は BuildLibraryIndex 内で std::map の走査順（＝アルバム名の
        // 昇順）のまま構築されているため、二分探索で高速に見つけられる
        // =======================================================
        const LibAlbum* FindLibAlbum(const pfc::string8& album_name) const
        {
            auto it = std::lower_bound(
                m_lib_albums.begin(), m_lib_albums.end(), album_name,
                [](const LibAlbum& a, const pfc::string8& name) {
                    return strcmp(a.album_name.c_str(), name.c_str()) < 0;
                });
            if (it != m_lib_albums.end() && it->album_name == album_name)
                return &(*it);
            return nullptr;
        }

        // =======================================================
        // 指定アルバムの全曲を、ディスク番号・トラック番号順に収集する
        // 通常は BuildLibraryIndex で事前に構築・ソート済みの全曲リストを
        // そのまま使う（クリックのたびにライブラリ全体を走査する必要がなく、
        // メインスレッドのブロッキングを避けられる）。
        // ライブラリインデックスに見当たらない想定外のケースのみ、
        // 従来通りライブラリ全体を走査するフォールバックを残す。
        // =======================================================
        void CollectAlbumTracks(const pfc::string8& album_name, metadb_handle_ptr fallback_track, metadb_handle_list& out_tracks)
        {
            const LibAlbum* found = FindLibAlbum(album_name);
            if (found && !found->tracks.empty())
            {
                for (auto& t : found->tracks)
                    out_tracks.add_item(t);
                return;
            }

            // フォールバック：ライブラリインデックスに見つからない場合のみ
            static_api_ptr_t<library_manager> lm;
            metadb_handle_list all_tracks;
            lm->get_all_items(all_tracks);

            for (t_size i = 0; i < all_tracks.get_count(); i++)
            {
                metadb_handle_ptr track = all_tracks[i];
                file_info_impl info;
                track->get_info(info);
                const char* album = info.meta_get("ALBUM", 0);
                pfc::string8 name = album ? album : "Unknown";

                if (name == album_name)
                    out_tracks.add_item(track);
            }

            if (out_tracks.get_count() == 0 && fallback_track.is_valid())
                out_tracks.add_item(fallback_track);

            out_tracks.sort_by_format("%discnumber%|%tracknumber%", nullptr);
        }

        HBITMAP GetOrLoadBgImage()
        {
            const char* path = g_cfg_bg_image_path.get();
            if (m_bgImageBitmap && m_bgImagePathLoaded == path)
                return m_bgImageBitmap;

            if (m_bgImageBitmap) { DeleteObject(m_bgImageBitmap); m_bgImageBitmap = NULL; }
            m_bgImagePathLoaded = path;
            m_bgImageBitmap = DecodeImageFileToBitmap(path);
            return m_bgImageBitmap;
        }

        HBITMAP GetOrLoadNoArtImage()
        {
            const char* path = g_cfg_noart_image_path.get();
            if (m_noArtImageBitmap && m_noArtImagePathLoaded == path)
                return m_noArtImageBitmap;

            if (m_noArtImageBitmap) { DeleteObject(m_noArtImageBitmap); m_noArtImageBitmap = NULL; }
            m_noArtImagePathLoaded = path;
            m_noArtImageBitmap = DecodeImageFileToBitmap(path);
            return m_noArtImageBitmap;
        }

        // =======================================================
        // 現在のマウス座標から、ホバー対象のエントリを判定する
        // （重なっている場合は手前優先＝末尾から探して最初に見つかったもの）
        // OnPaint（描画用）とOnMouseMove（変化検知用）の両方から呼ばれる
        // =======================================================
        QueueEntry* FindHoverEntry(int mouseX, int mouseY)
        {
            if (m_queue.empty()) return nullptr;

            RECT rc = GetClientRectHelper();
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            float base_art_size = (float)GetArtSize(rc);
            const float max_scale = 1.2f;
            float max_art_size = base_art_size * max_scale;

            float textBlockHeight = (float)GetTextBlockHeight();
            float content_height = max_art_size + textBlockHeight;

            float draw_y = ((float)height - content_height) / 2.0f;
            if (draw_y < 5.0f) draw_y = 5.0f;

            float center_x = (float)width / 2.0f;
            float max_dist = (float)width / 2.0f;

            for (auto it = m_queue.rbegin(); it != m_queue.rend(); ++it)
            {
                auto& e = *it;
                float base_draw_x = e.x;
                if (base_draw_x + base_art_size < 0 || base_draw_x > width) continue;

                float scale = 1.0f;
                if (g_cfg_use_perspective.get())
                {
                    float entry_center = base_draw_x + base_art_size / 2.0f;
                    float dist = fabs(entry_center - center_x);
                    float t = dist / max_dist;
                    if (t > 1.0f) t = 1.0f;
                    scale = 1.2f - t * 0.2f;
                }

                float art_size = base_art_size * scale;
                if (art_size < 10.0f) art_size = 10.0f;
                art_size = (float)((int)(art_size / 2.0f + 0.5f) * 2);

                float draw_x = base_draw_x + (base_art_size - art_size) / 2.0f;
                float draw_y_adj = draw_y + (max_art_size - art_size) / 2.0f;

                if ((float)mouseX >= draw_x && (float)mouseX <= draw_x + art_size &&
                    (float)mouseY >= draw_y_adj && (float)mouseY <= draw_y_adj + art_size)
                {
                    return &e;
                }
            }
            return nullptr;
        }

        // =======================================================
        // マウスオーバー枠表示のため、現在のマウス位置を記憶する
        // ウィンドウに入った直後にTrackMouseEventを呼び、ウィンドウから
        // 出た際にWM_MOUSELEAVEを受け取れるようにする
        // =======================================================
        void OnMouseMove(int x, int y)
        {
            if (!m_mouseInWindow)
            {
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = m_hwnd;
                TrackMouseEvent(&tme);
            }

            m_mouseInWindow = true;
            m_mouseX = x;
            m_mouseY = y;

            // ホバー枠の再描画は、ホバー中のエントリが実際に切り替わった
            // 時だけ行う。スクロールアニメーション自体は既にタイマーで
            // 毎フレーム再描画されているため、マウスが動くたびに（ホバー
            // 対象が変わっていなくても）無条件でInvalidateRectしていた
            // 従来の実装は、その再描画に上乗せする形で過剰な再描画を
            // 発生させ、流れが一瞬止まって見える原因になっていた。
            if (g_cfg_show_hover_frame.get())
            {
                QueueEntry* newHover = FindHoverEntry(x, y);
                if (newHover != m_lastHoverEntry)
                {
                    m_lastHoverEntry = newHover;
                    if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
                }
            }
        }

        // =======================================================
        // 指定アルバムの全曲を新規プレイリストに追加する（再生はしない）
        // 戻り値：作成したプレイリストのインデックス（失敗時はpfc::infinite_size）
        // =======================================================
        t_size AddAlbumToNewPlaylist(const pfc::string8& album_name, metadb_handle_ptr track)
        {
            metadb_handle_list album_tracks;
            CollectAlbumTracks(album_name, track, album_tracks);
            if (album_tracks.get_count() == 0) return pfc::infinite_size;

            static_api_ptr_t<playlist_manager> pm;

            t_size new_playlist = pm->create_playlist(
                "Album Train", pfc::infinite_size, pfc::infinite_size);
            if (new_playlist == pfc::infinite_size) return pfc::infinite_size;

            pm->playlist_add_items(new_playlist, album_tracks,
                bit_array_false());
            pm->set_active_playlist(new_playlist);

            return new_playlist;
        }

        // =======================================================
        // シングル左クリック：クリックしたアルバムの全曲を新規プレイリストに
        // 追加するのみ（再生はしない）
        // =======================================================
        void OnLButtonDown(int x, int y)
        {
            pfc::string8 album_name;
            metadb_handle_ptr track;
            if (!HitTestAlbum(x, y, album_name, track)) return;

            t_size new_playlist = AddAlbumToNewPlaylist(album_name, track);
            if (new_playlist == pfc::infinite_size) return;

            // ダブルクリックとして検知された場合に、このプレイリストを
            // 再利用できるよう覚えておく
            m_lastClickPlaylist = new_playlist;
            m_lastClickAlbumName = album_name;
        }

        // =======================================================
        // ダブル左クリック：プレイリストに追加した上で1曲目を再生する
        // Windowsはダブルクリックの直前に必ずシングルクリックのメッセージ
        // (WM_LBUTTONDOWN)も送るため、OnLButtonDownによる追加が既に
        // 行われている。同じアルバムに対するものであれば、そのプレイリスト
        // をそのまま再利用し、二重にプレイリストが作られないようにする
        // =======================================================
        void OnLButtonDblClk(int x, int y)
        {
            pfc::string8 album_name;
            metadb_handle_ptr track;
            if (!HitTestAlbum(x, y, album_name, track)) return;

            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pfc::infinite_size;

            if (m_lastClickPlaylist != pfc::infinite_size &&
                m_lastClickAlbumName == album_name &&
                m_lastClickPlaylist < pm->get_playlist_count())
            {
                playlist = m_lastClickPlaylist;
            }
            else
            {
                // 直前のシングルクリック処理が見つからない場合
                // （極端に短い間隔でのダブルクリック等）のフォールバック
                playlist = AddAlbumToNewPlaylist(album_name, track);
                if (playlist == pfc::infinite_size) return;
            }

            pm->set_active_playlist(playlist);
            pm->set_playing_playlist(playlist);
            pm->playlist_set_focus_item(playlist, 0);
            pm->playlist_execute_default_action(playlist, 0);

            m_lastClickPlaylist = pfc::infinite_size;
            m_lastClickAlbumName = "";
        }

        // =======================================================
        // マウスホイールでスクロール
        // =======================================================
        void OnMouseWheel(int delta)
        {
            if (!g_cfg_allow_wheel_scroll.get()) return;
            if (m_queue.empty()) return;

            RECT rc = GetClientRectHelper();
            int spacing = GetSpacing(rc);
            int width = rc.right - rc.left;

            float jump = (float)spacing;
            if (delta > 0)
                for (auto& e : m_queue) e.x += jump;
            else
                for (auto& e : m_queue) e.x -= jump;

            ExtendQueue(spacing, width);  // タイマー任せにせず、ホイール操作直後に整合性を取る

            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
        }

        // =======================================================
        // 設定変更時の共通処理
        // 見た目に影響する設定が変わったら、キューを作り直して再描画する
        // =======================================================
        void OnSettingsChanged()
        {
            m_queue.clear();
            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
        }

        // =======================================================
        // ステージング領域の内容を実際の設定変数に反映する
        // Apply / OK ボタン押下時にのみ呼ばれる
        // =======================================================
        static void CommitSettings(HWND hDlg, SettingsDialogContext* ctx)
        {
            g_cfg_scroll_speed = ctx->staging.speed;
            g_cfg_show_no_artwork = ctx->staging.showNoArt;
            g_cfg_allow_wheel_scroll = ctx->staging.wheelScroll;
            g_cfg_show_album_name = ctx->staging.showAlbumName;
            g_cfg_use_perspective = ctx->staging.perspective;
            g_cfg_scroll_right_to_left = ctx->staging.scrollRTL;
            g_cfg_use_second_line = ctx->staging.useSecondLine;
            g_cfg_use_theme_colors = ctx->staging.useThemeColors;
            g_cfg_stability_mode = ctx->staging.stabilityMode;
            g_cfg_art_text_gap = ctx->staging.gap1;
            g_cfg_line_gap = ctx->staging.gap2;
            g_cfg_album_font = ctx->staging.font;
            g_cfg_bg_color = ctx->staging.bgColor;
            g_cfg_text_color = ctx->staging.textColor;
            g_cfg_noart_color = ctx->staging.noArtColor;
            g_cfg_use_bg_image = ctx->staging.bgUseImage;
            g_cfg_bg_image_path = ctx->staging.bgImagePath;
            g_cfg_use_noart_image = ctx->staging.noArtUseImage;
            g_cfg_noart_image_path = ctx->staging.noArtImagePath;
            g_cfg_artwork_quality = ctx->staging.artworkQuality;
            g_cfg_bg_image_quality = ctx->staging.bgImageQuality;
            g_cfg_fix_text_position = ctx->staging.fixTextPosition;
            g_cfg_show_hover_frame = ctx->staging.showHoverFrame;
            g_cfg_hover_frame_color = ctx->staging.hoverFrameColor;

            // フォーマット文字列は、コミット時にエディットボックスから直接取得する
            HWND editBox = GetDlgItem(hDlg, IDC_FORMAT_EDIT);
            TCHAR buf[256] = {};
            GetWindowText(editBox, buf, 256);
            pfc::stringcvt::string_utf8_from_os text_utf8(buf);
            g_cfg_album_format = text_utf8.get_ptr();

            if (ctx->staging.useSecondLine)
            {
                HWND editBox2 = GetDlgItem(hDlg, IDC_FORMAT2_EDIT);
                TCHAR buf2[256] = {};
                GetWindowText(editBox2, buf2, 256);
                pfc::stringcvt::string_utf8_from_os text2_utf8(buf2);
                g_cfg_album_format2 = text2_utf8.get_ptr();
            }

            if (ctx->panel)
                ctx->panel->OnSettingsChanged();
        }

        // =======================================================
        // 「Text」グループ内の各コントロールの有効/無効状態を、
        // 現在のステージング値に基づいてまとめて再計算する。
        // 複数の条件が絡み合うため、個別にEnableWindowを呼ぶのではなく
        // 必ずこの関数を経由することで、重複・矛盾を防ぐ。
        // =======================================================
        static void UpdateTextGroupEnables(HWND hDlg, SettingsDialogContext* ctx)
        {
            if (!ctx) return;

            bool showName = ctx->staging.showAlbumName;
            bool useSecondLine = ctx->staging.useSecondLine;
            bool fixTextPos = ctx->staging.fixTextPosition;
            bool useTheme = ctx->staging.useThemeColors;

            EnableWindow(GetDlgItem(hDlg, IDC_FIXTEXTPOS_CHECK), showName);
            EnableWindow(GetDlgItem(hDlg, IDC_FONT_BUTTON), showName);
            EnableWindow(GetDlgItem(hDlg, IDC_TEXT_COLOR_BTN), showName && !useTheme);
            EnableWindow(GetDlgItem(hDlg, IDC_FORMAT_EDIT), showName);
            EnableWindow(GetDlgItem(hDlg, IDC_GAP1_EDIT), showName && !fixTextPos);
            EnableWindow(GetDlgItem(hDlg, IDC_GAP1_SPIN), showName && !fixTextPos);
            EnableWindow(GetDlgItem(hDlg, IDC_ADD_LINE_BTN), showName);
            EnableWindow(GetDlgItem(hDlg, IDC_FORMAT2_EDIT), showName && useSecondLine);
            EnableWindow(GetDlgItem(hDlg, IDC_GAP2_EDIT), showName && useSecondLine);
            EnableWindow(GetDlgItem(hDlg, IDC_GAP2_SPIN), showName && useSecondLine);
        }

        // =======================================================
        // 設定ダイアログのウィンドウプロシージャ
        // =======================================================
        static LRESULT CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
        {
            switch (msg)
            {
            case WM_INITDIALOG:
            {
                SettingsDialogContext* ctx = new SettingsDialogContext();
                ctx->panel = (AlbumTrainPanel*)lp;

                // 現在の設定値をステージング領域にコピーする
                ctx->staging.speed = (int)g_cfg_scroll_speed.get();
                ctx->staging.showNoArt = g_cfg_show_no_artwork.get();
                ctx->staging.wheelScroll = g_cfg_allow_wheel_scroll.get();
                ctx->staging.showAlbumName = g_cfg_show_album_name.get();
                ctx->staging.perspective = g_cfg_use_perspective.get();
                ctx->staging.scrollRTL = g_cfg_scroll_right_to_left.get();
                ctx->staging.useSecondLine = g_cfg_use_second_line.get();
                ctx->staging.useThemeColors = g_cfg_use_theme_colors.get();
                ctx->staging.stabilityMode = g_cfg_stability_mode.get();
                ctx->staging.gap1 = (int)g_cfg_art_text_gap.get();
                ctx->staging.gap2 = (int)g_cfg_line_gap.get();
                ctx->staging.font = g_cfg_album_font.get();
                ctx->staging.bgColor = g_cfg_bg_color.get();
                ctx->staging.textColor = g_cfg_text_color.get();
                ctx->staging.noArtColor = g_cfg_noart_color.get();
                ctx->staging.bgUseImage = g_cfg_use_bg_image.get();
                ctx->staging.bgImagePath = g_cfg_bg_image_path.get();
                ctx->staging.noArtUseImage = g_cfg_use_noart_image.get();
                ctx->staging.noArtImagePath = g_cfg_noart_image_path.get();
                ctx->staging.fixTextPosition = g_cfg_fix_text_position.get();
                ctx->staging.artworkQuality = (int)g_cfg_artwork_quality.get();
                ctx->staging.bgImageQuality = (int)g_cfg_bg_image_quality.get();
                ctx->staging.showHoverFrame = g_cfg_show_hover_frame.get();
                ctx->staging.hoverFrameColor = g_cfg_hover_frame_color.get();


                SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)ctx);

                // -----------------------------------------------
                // フォント実測値から、見切れないコントロールサイズを算出する
                // -----------------------------------------------
                int lineH = GetDialogFontLineHeight(hDlg);
                int rowH = lineH + 8;
                int editH = lineH + 10;
                int labelH = lineH + 4;
                int titleSpace = lineH + 10;
                int groupPad = 8;
                int groupGap = 10;
                int colGap = 10;
                int sliderH = max(28, rowH + 4);

                const int MARGIN = 10;
                const int GROUP_W = 270;
                const int CONTENT_X = 20;
                const int CONTENT_W = 250;

                // 右列用のX座標（左列のグループ幅＋列間の隙間ぶんだけ右にずらす）
                const int MARGIN_R = MARGIN + GROUP_W + colGap;
                const int CONTENT_X_R = MARGIN_R + 10;

                // 色設定の行レイアウト用定数（左列・右列で共用。X位置だけ列ごとに使い分ける）
                const int SWATCH_LABEL_W = 95;
                const int SWATCH_W = 40;
                const int SWATCH_GAP = 6;
                const int SWATCH_BTN_X = CONTENT_X + SWATCH_LABEL_W + SWATCH_GAP + SWATCH_W + SWATCH_GAP;
                const int SWATCH_BTN_X_R = CONTENT_X_R + SWATCH_LABEL_W + SWATCH_GAP + SWATCH_W + SWATCH_GAP;
                const int SWATCH_BTN_W = CONTENT_W - SWATCH_LABEL_W - SWATCH_GAP - SWATCH_W - SWATCH_GAP;
                const int swatchH = rowH - 6;

                int yLeft = 8;   // 左列の現在の書き込みY座標
                int yRight = 8;  // 右列の現在の書き込みY座標

                // ---- グループ1（左列）：Mode ----
                int modeGroupH = titleSpace + rowH * 2 + groupPad;
                CreateWindowEx(0, _T("BUTTON"), _T("Mode"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN, yLeft, GROUP_W, modeGroupH, hDlg, NULL, NULL, NULL);

                HWND modeStability = CreateWindowEx(0, _T("BUTTON"), _T("Stability Focused (Lightweight)"),
                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                    CONTENT_X, yLeft + titleSpace, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_MODE_STABILITY,
                    NULL, NULL);

                HWND modeCustom = CreateWindowEx(0, _T("BUTTON"), _T("Customization Focused"),
                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                    CONTENT_X, yLeft + titleSpace + rowH, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_MODE_CUSTOM,
                    NULL, NULL);

                if (ctx->staging.stabilityMode)
                    SendMessage(modeStability, BM_SETCHECK, BST_CHECKED, 0);
                else
                    SendMessage(modeCustom, BM_SETCHECK, BST_CHECKED, 0);

                yLeft += modeGroupH + groupGap;

                // ---- グループ2（右列）：Theme ----
                int themeGroupH = titleSpace + rowH * 1 + groupPad;
                CreateWindowEx(0, _T("BUTTON"), _T("Theme"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN_R, yRight, GROUP_W, themeGroupH, hDlg, NULL, NULL, NULL);

                HWND themeCheck = CreateWindowEx(0, _T("BUTTON"), _T("Use Theme Colours"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X_R, yRight + titleSpace, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_THEME_CHECK,
                    NULL, NULL);
                SendMessage(themeCheck, BM_SETCHECK,
                    ctx->staging.useThemeColors ? BST_CHECKED : BST_UNCHECKED, 0);

                yRight += themeGroupH + groupGap;

                // ---- グループ3（左列）：Artwork ----
                int artworkGroupH = titleSpace + rowH * 8 + groupPad;
                CreateWindowEx(0, _T("BUTTON"), _T("Artwork"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN, yLeft, GROUP_W, artworkGroupH, hDlg, NULL, NULL, NULL);

                int naY = yLeft + titleSpace;

                // --- Artwork Quality ---
                CreateWindowEx(0, _T("STATIC"), _T("Artwork Quality:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, naY + (rowH - labelH) / 2, 110, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND artworkQualityCombo = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL,
                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                    CONTENT_X + 110, naY, CONTENT_W - 110, rowH * 5, hDlg, (HMENU)(INT_PTR)IDC_ARTWORK_QUALITY_COMBO,
                    NULL, NULL);
                // v4.0.0：Ultra（GDI+高品質補間）を追加。既存設定との互換性を保つため、
                // 内部値（cfg保存値）はHigh=0/Middle=1/Low=2のまま変更せず、
                // 新設のUltraには4番目の値=3を割り当てる。表示順序（Ultraが先頭）と
                // 内部値の並びが一致しないため、CB_SETITEMDATAで表示位置↔内部値を
                // 対応付ける
                SendMessage(artworkQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("Ultra (Beta)"));
                SendMessage(artworkQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("High"));
                SendMessage(artworkQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("Middle"));
                SendMessage(artworkQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("Low"));
                SendMessage(artworkQualityCombo, CB_SETITEMDATA, 0, (LPARAM)3);  // Ultra → 内部値3
                SendMessage(artworkQualityCombo, CB_SETITEMDATA, 1, (LPARAM)0);  // High  → 内部値0
                SendMessage(artworkQualityCombo, CB_SETITEMDATA, 2, (LPARAM)1);  // Middle→ 内部値1
                SendMessage(artworkQualityCombo, CB_SETITEMDATA, 3, (LPARAM)2);  // Low   → 内部値2
                {
                    int initialIndex = 1; // 既定はHighの表示位置
                    switch (ctx->staging.artworkQuality)
                    {
                    case 3: initialIndex = 0; break; // Ultra
                    case 0: initialIndex = 1; break; // High
                    case 1: initialIndex = 2; break; // Middle
                    case 2: initialIndex = 3; break; // Low
                    }
                    SendMessage(artworkQualityCombo, CB_SETCURSEL, initialIndex, 0);
                }
                EnableWindow(artworkQualityCombo, !ctx->staging.stabilityMode);
                naY += rowH;

                // --- Show Hover Frame（マウスオーバー時にアートワークへ枠を表示） ---
                HWND hoverFrameCheck = CreateWindowEx(0, _T("BUTTON"), _T("Show Hover Frame"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, naY, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_HOVER_FRAME_CHECK,
                    NULL, NULL);
                SendMessage(hoverFrameCheck, BM_SETCHECK,
                    ctx->staging.showHoverFrame ? BST_CHECKED : BST_UNCHECKED, 0);
                naY += rowH;

                // --- Show Hover Frame色変更UI ---
                CreateWindowEx(0, _T("STATIC"), _T("Colour:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, naY + (rowH - labelH) / 2, SWATCH_LABEL_W, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND hoverFrameSwatch = CreateWindowEx(0, _T("STATIC"), NULL,
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X + SWATCH_LABEL_W + SWATCH_GAP, naY + (rowH - swatchH) / 2, SWATCH_W, swatchH,
                    hDlg, (HMENU)(INT_PTR)IDC_HOVER_FRAME_COLOR_SWATCH, NULL, NULL);
                SetWindowSubclass(hoverFrameSwatch, ColorSwatchSubclassProc, 0, (DWORD_PTR)&ctx->hHoverFrameBrush);

                HWND hoverFrameColorBtn = CreateWindowEx(0, _T("BUTTON"), _T("Change..."),
                    WS_CHILD | WS_VISIBLE,
                    SWATCH_BTN_X, naY, SWATCH_BTN_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_HOVER_FRAME_COLOR_BTN,
                    NULL, NULL);
                naY += rowH;

                // --- Perspective Effect ---
                HWND perspectiveCheck = CreateWindowEx(0, _T("BUTTON"), _T("Perspective Effect"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, naY, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_PERSPECTIVE_CHECK,
                    NULL, NULL);
                SendMessage(perspectiveCheck, BM_SETCHECK,
                    ctx->staging.perspective ? BST_CHECKED : BST_UNCHECKED, 0);
                naY += rowH;

                // --- Show Albums Without Artwork（以下、既存のまま） ---
                HWND showNoArtCheck = CreateWindowEx(0, _T("BUTTON"), _T("Show Albums Without Artwork"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, naY, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_SHOWNOART_CHECK,
                    NULL, NULL);
                SendMessage(showNoArtCheck, BM_SETCHECK,
                    ctx->staging.showNoArt ? BST_CHECKED : BST_UNCHECKED, 0);
                EnableWindow(showNoArtCheck, !ctx->staging.stabilityMode);
                naY += rowH;

                // --- No-Artwork色変更UI ---
                CreateWindowEx(0, _T("STATIC"), _T("Colour:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, naY + (rowH - labelH) / 2, SWATCH_LABEL_W, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND noArtSwatch = CreateWindowEx(0, _T("STATIC"), NULL,
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X + SWATCH_LABEL_W + SWATCH_GAP, naY + (rowH - swatchH) / 2, SWATCH_W, swatchH,
                    hDlg, (HMENU)(INT_PTR)IDC_NOART_COLOR_SWATCH, NULL, NULL);
                SetWindowSubclass(noArtSwatch, ColorSwatchSubclassProc, 0, (DWORD_PTR)&ctx->hNoArtBrush);

                HWND noArtColorBtn = CreateWindowEx(0, _T("BUTTON"), _T("Change..."),
                    WS_CHILD | WS_VISIBLE,
                    SWATCH_BTN_X, naY, SWATCH_BTN_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_NOART_COLOR_BTN,
                    NULL, NULL);
                naY += rowH;

                // --- No-Artwork：画像使用チェックボックス ---
                HWND noArtModeImage = CreateWindowEx(0, _T("BUTTON"), _T("Use Image"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, naY, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_NOART_MODE_IMAGE,
                    NULL, NULL);
                SendMessage(noArtModeImage, BM_SETCHECK,
                    ctx->staging.noArtUseImage ? BST_CHECKED : BST_UNCHECKED, 0);
                naY += rowH;

                // --- No-Artwork画像選択UI ---
                HWND noArtImageEdit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                    CONTENT_X, naY, CONTENT_W - 80, editH, hDlg, (HMENU)(INT_PTR)IDC_NOART_IMAGE_EDIT,
                    NULL, NULL);
                {
                    pfc::stringcvt::string_os_from_utf8 path_t(
                        ctx->staging.noArtImagePath.get_length() ? ctx->staging.noArtImagePath.get_ptr() : "(no image selected)");
                    SetWindowText(noArtImageEdit, path_t);
                }

                HWND noArtImageBrowseBtn = CreateWindowEx(0, _T("BUTTON"), _T("Browse..."),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X + CONTENT_W - 75, naY, 75, editH, hDlg, (HMENU)(INT_PTR)IDC_NOART_IMAGE_BROWSE_BTN,
                    NULL, NULL);

                yLeft += artworkGroupH + groupGap;

                // ---- グループ4（右列）：Scroll ----
                int scrollGroupH = titleSpace + sliderH + 4 + labelH + 4 + rowH * 3 + groupPad;
                CreateWindowEx(0, _T("BUTTON"), _T("Scroll"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN_R, yRight, GROUP_W, scrollGroupH, hDlg, NULL, NULL, NULL);

                int sy = yRight + titleSpace;

                HWND dirLtr = CreateWindowEx(0, _T("BUTTON"), _T("Scroll Left to Right"),
                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                    CONTENT_X_R, sy, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_DIR_LTR,
                    NULL, NULL);
                sy += rowH;

                HWND dirRtl = CreateWindowEx(0, _T("BUTTON"), _T("Scroll Right to Left"),
                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                    CONTENT_X_R, sy, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_DIR_RTL,
                    NULL, NULL);
                sy += rowH;

                if (ctx->staging.scrollRTL)
                    SendMessage(dirRtl, BM_SETCHECK, BST_CHECKED, 0);
                else
                    SendMessage(dirLtr, BM_SETCHECK, BST_CHECKED, 0);

                CreateWindowEx(0, TRACKBAR_CLASS, NULL,
                    WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
                    CONTENT_X_R, sy, CONTENT_W, sliderH, hDlg, (HMENU)(INT_PTR)IDC_SPEED_SLIDER,
                    NULL, NULL);
                sy += sliderH + 4;

                CreateWindowEx(0, _T("STATIC"), _T("Scroll Speed"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X_R, sy, CONTENT_W, labelH, hDlg, (HMENU)(INT_PTR)IDC_SPEED_LABEL,
                    NULL, NULL);
                sy += labelH + 4;

                HWND wheelCheck = CreateWindowEx(0, _T("BUTTON"), _T("Allow Mouse Wheel Scrolling"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X_R, sy, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_WHEEL_CHECK,
                    NULL, NULL);
                SendMessage(wheelCheck, BM_SETCHECK,
                    ctx->staging.wheelScroll ? BST_CHECKED : BST_UNCHECKED, 0);

                yRight += scrollGroupH + groupGap;   // ← この行が抜けていました

                // ---- グループ5（左列）：Text ----
                int textGroupH = titleSpace
                    + rowH                    // Show Text Under Artwork
                    + rowH                    // Fix Text Position
                    + rowH                    // Font button
                    + rowH                    // Colour row
                    + labelH + editH + 4      // First Line Text label + edit
                    + editH + 4               // Artwork-Text Gap
                    + rowH                    // Use Second Line
                    + labelH + editH + 4      // Second Line Text label + edit
                    + editH                   // Line Gap
                    + groupPad;

                CreateWindowEx(0, _T("BUTTON"), _T("Text"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN, yLeft, GROUP_W, textGroupH, hDlg, NULL, NULL, NULL);

                int ay = yLeft + titleSpace;

                // --- Show Text Under Artwork ---
                HWND showNameCheck = CreateWindowEx(0, _T("BUTTON"), _T("Show Text Under Artwork"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, ay, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_SHOWNAME_CHECK,
                    NULL, NULL);
                SendMessage(showNameCheck, BM_SETCHECK,
                    ctx->staging.showAlbumName ? BST_CHECKED : BST_UNCHECKED, 0);
                ay += rowH;

                // --- Fix Text Position ---
                HWND fixTextPosCheck = CreateWindowEx(0, _T("BUTTON"), _T("Fix Text Position"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, ay, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_FIXTEXTPOS_CHECK,
                    NULL, NULL);
                SendMessage(fixTextPosCheck, BM_SETCHECK,
                    ctx->staging.fixTextPosition ? BST_CHECKED : BST_UNCHECKED, 0);
                ay += rowH;

                // --- Font ---
                CreateWindowEx(0, _T("BUTTON"), _T("Font..."),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay, 100, rowH, hDlg, (HMENU)(INT_PTR)IDC_FONT_BUTTON,
                    NULL, NULL);
                ay += rowH;

                CreateWindowEx(0, _T("STATIC"), _T("Colour:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay + (rowH - labelH) / 2, SWATCH_LABEL_W, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND textSwatch = CreateWindowEx(0, _T("STATIC"), NULL,
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X + SWATCH_LABEL_W + SWATCH_GAP, ay + (rowH - swatchH) / 2, SWATCH_W, swatchH,
                    hDlg, (HMENU)(INT_PTR)IDC_TEXT_COLOR_SWATCH, NULL, NULL);
                SetWindowSubclass(textSwatch, ColorSwatchSubclassProc, 0, (DWORD_PTR)&ctx->hTextBrush);

                HWND textColorBtn = CreateWindowEx(0, _T("BUTTON"), _T("Change..."),
                    WS_CHILD | WS_VISIBLE,
                    SWATCH_BTN_X, ay, SWATCH_BTN_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_TEXT_COLOR_BTN,
                    NULL, NULL);
                ay += rowH;

                CreateWindowEx(0, _T("STATIC"), _T("First Line Text:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay, CONTENT_W, labelH, hDlg, (HMENU)(INT_PTR)IDC_FORMAT_LABEL,
                    NULL, NULL);
                ay += labelH;

                HWND editBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    CONTENT_X, ay, CONTENT_W, editH, hDlg, (HMENU)(INT_PTR)IDC_FORMAT_EDIT,
                    NULL, NULL);
                {
                    pfc::stringcvt::string_os_from_utf8 fmt_t(g_cfg_album_format.get());
                    SetWindowText(editBox, fmt_t);
                }
                ay += editH + 4;

                CreateWindowEx(0, _T("STATIC"), _T("Artwork-Text Gap (px):"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay + (editH - labelH) / 2, 170, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND gap1Edit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_READONLY,
                    205, ay, 45, editH, hDlg, (HMENU)(INT_PTR)IDC_GAP1_EDIT,
                    NULL, NULL);

                HWND gap1Spin = CreateWindowEx(0, UPDOWN_CLASS, NULL,
                    WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
                    0, 0, 0, 0, hDlg, (HMENU)(INT_PTR)IDC_GAP1_SPIN,
                    NULL, NULL);
                SendMessage(gap1Spin, UDM_SETBUDDY, (WPARAM)gap1Edit, 0);
                SendMessage(gap1Spin, UDM_SETRANGE, 0, MAKELONG(10, 1));
                SendMessage(gap1Spin, UDM_SETPOS, 0, MAKELONG(ctx->staging.gap1, 0));
                ay += editH + 4;

                HWND addLineCheck = CreateWindowEx(0, _T("BUTTON"), _T("Use Second Line"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X, ay, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_ADD_LINE_BTN,
                    NULL, NULL);
                SendMessage(addLineCheck, BM_SETCHECK,
                    ctx->staging.useSecondLine ? BST_CHECKED : BST_UNCHECKED, 0);
                ay += rowH;

                CreateWindowEx(0, _T("STATIC"), _T("Second Line Text:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay, CONTENT_W, labelH, hDlg, (HMENU)(INT_PTR)IDC_FORMAT2_LABEL,
                    NULL, NULL);
                ay += labelH;

                HWND editBox2 = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    CONTENT_X, ay, CONTENT_W, editH, hDlg, (HMENU)(INT_PTR)IDC_FORMAT2_EDIT,
                    NULL, NULL);
                {
                    pfc::stringcvt::string_os_from_utf8 fmt2_t(g_cfg_album_format2.get());
                    SetWindowText(editBox2, fmt2_t);
                }
                ay += editH + 4;

                CreateWindowEx(0, _T("STATIC"), _T("Line Gap (px):"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X, ay + (editH - labelH) / 2, 170, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND gap2Edit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_READONLY,
                    205, ay, 45, editH, hDlg, (HMENU)(INT_PTR)IDC_GAP2_EDIT,
                    NULL, NULL);

                HWND gap2Spin = CreateWindowEx(0, UPDOWN_CLASS, NULL,
                    WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
                    0, 0, 0, 0, hDlg, (HMENU)(INT_PTR)IDC_GAP2_SPIN,
                    NULL, NULL);
                SendMessage(gap2Spin, UDM_SETBUDDY, (WPARAM)gap2Edit, 0);
                SendMessage(gap2Spin, UDM_SETRANGE, 0, MAKELONG(10, 1));
                SendMessage(gap2Spin, UDM_SETPOS, 0, MAKELONG(ctx->staging.gap2, 0));

                yLeft += textGroupH + groupGap;

                // ---- グループ6（右列）：Background ----
                int backgroundGroupH = titleSpace + rowH * 3 + (editH + 4) + groupPad;
                CreateWindowEx(0, _T("BUTTON"), _T("Background"),
                    WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    MARGIN_R, yRight, GROUP_W, backgroundGroupH, hDlg, NULL, NULL, NULL);

                int bgCy = yRight + titleSpace;

                CreateWindowEx(0, _T("STATIC"), _T("Colour:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X_R, bgCy + (rowH - labelH) / 2, SWATCH_LABEL_W, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND bgSwatch = CreateWindowEx(0, _T("STATIC"), NULL,
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X_R + SWATCH_LABEL_W + SWATCH_GAP, bgCy + (rowH - swatchH) / 2, SWATCH_W, swatchH,
                    hDlg, (HMENU)(INT_PTR)IDC_BG_COLOR_SWATCH, NULL, NULL);
                SetWindowSubclass(bgSwatch, ColorSwatchSubclassProc, 0, (DWORD_PTR)&ctx->hBgBrush);

                HWND bgColorBtn = CreateWindowEx(0, _T("BUTTON"), _T("Change..."),
                    WS_CHILD | WS_VISIBLE,
                    SWATCH_BTN_X_R, bgCy, SWATCH_BTN_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_BG_COLOR_BTN,
                    NULL, NULL);
                bgCy += rowH;

                HWND bgModeImage = CreateWindowEx(0, _T("BUTTON"), _T("Use Image"),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    CONTENT_X_R, bgCy, CONTENT_W, rowH, hDlg, (HMENU)(INT_PTR)IDC_BG_MODE_IMAGE,
                    NULL, NULL);
                SendMessage(bgModeImage, BM_SETCHECK,
                    ctx->staging.bgUseImage ? BST_CHECKED : BST_UNCHECKED, 0);
                bgCy += rowH;

                HWND bgImageEdit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                    CONTENT_X_R, bgCy, CONTENT_W - 80, editH, hDlg, (HMENU)(INT_PTR)IDC_BG_IMAGE_EDIT,
                    NULL, NULL);
                {
                    pfc::stringcvt::string_os_from_utf8 path_t(
                        ctx->staging.bgImagePath.get_length() ? ctx->staging.bgImagePath.get_ptr() : "(no image selected)");
                    SetWindowText(bgImageEdit, path_t);
                }

                HWND bgImageBrowseBtn = CreateWindowEx(0, _T("BUTTON"), _T("Browse..."),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X_R + CONTENT_W - 75, bgCy, 75, editH, hDlg, (HMENU)(INT_PTR)IDC_BG_IMAGE_BROWSE_BTN,
                    NULL, NULL);
                bgCy += editH + 4;

                // --- Image Quality（Backgroundグループ専用、Artworkの品質設定とは独立） ---
                CreateWindowEx(0, _T("STATIC"), _T("Image Quality:"),
                    WS_CHILD | WS_VISIBLE,
                    CONTENT_X_R, bgCy + (rowH - labelH) / 2, 110, labelH, hDlg, NULL,
                    NULL, NULL);

                HWND bgImageQualityCombo = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL,
                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                    CONTENT_X_R + 110, bgCy, CONTENT_W - 110, rowH * 5, hDlg, (HMENU)(INT_PTR)IDC_BG_IMAGE_QUALITY_COMBO,
                    NULL, NULL);
                SendMessage(bgImageQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("High"));
                SendMessage(bgImageQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("Middle"));
                SendMessage(bgImageQualityCombo, CB_ADDSTRING, 0, (LPARAM)_T("Low"));
                SendMessage(bgImageQualityCombo, CB_SETCURSEL, ctx->staging.bgImageQuality, 0);
                EnableWindow(bgImageQualityCombo, !ctx->staging.stabilityMode&& ctx->staging.bgUseImage);

                // 色プレビュー用のブラシを、現在のステージング値で作成する
                ctx->hBgBrush = CreateSolidBrush(ctx->staging.bgColor);
                ctx->hTextBrush = CreateSolidBrush(ctx->staging.textColor);
                ctx->hNoArtBrush = CreateSolidBrush(ctx->staging.noArtColor);
                ctx->hHoverFrameBrush = CreateSolidBrush(ctx->staging.hoverFrameColor);

                // テーマ追従・画像モードの状態に応じて、色ボタン／画像ボタンの有効性をまとめて初期反映する
                bool useTheme = ctx->staging.useThemeColors;
                EnableWindow(bgColorBtn, !useTheme && !ctx->staging.bgUseImage);
                EnableWindow(bgImageBrowseBtn, ctx->staging.bgUseImage);
                EnableWindow(textColorBtn, !useTheme);
                EnableWindow(noArtColorBtn, !useTheme && !ctx->staging.noArtUseImage);
                EnableWindow(noArtImageBrowseBtn, ctx->staging.noArtUseImage);

                yRight += backgroundGroupH + groupGap;

                // ---- Cancel / Apply / OK ボタン（ウィンドウ右下に配置） ----
                int btnH = rowH + 2;
                const int BTN_W = 65;
                const int BTN_GAP = 5;
                int btnBlockW = BTN_W * 3 + BTN_GAP * 2;

                int contentBottomY = max(yLeft, yRight);
                int desiredClientW = MARGIN_R + GROUP_W + MARGIN;  // 左余白＋左列＋列間＋右列＋右余白
                int btnY = contentBottomY;
                int btnRight = desiredClientW - MARGIN;
                int btnX0 = btnRight - btnBlockW;

                CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
                    WS_CHILD | WS_VISIBLE,
                    btnX0, btnY, BTN_W, btnH, hDlg, (HMENU)(INT_PTR)IDCANCEL,
                    NULL, NULL);

                CreateWindowEx(0, _T("BUTTON"), _T("Apply"),
                    WS_CHILD | WS_VISIBLE,
                    btnX0 + (BTN_W + BTN_GAP), btnY, BTN_W, btnH, hDlg, (HMENU)(INT_PTR)IDC_APPLY_BTN,
                    NULL, NULL);

                CreateWindowEx(0, _T("BUTTON"), _T("OK"),
                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    btnX0 + (BTN_W + BTN_GAP) * 2, btnY, BTN_W, btnH, hDlg, (HMENU)(INT_PTR)IDOK,
                    NULL, NULL);

                int y = btnY + btnH + 10;

                // スライダーの範囲設定（1～10）
                HWND slider = GetDlgItem(hDlg, IDC_SPEED_SLIDER);
                SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(1, 10));
                SendMessage(slider, TBM_SETPOS, TRUE, ctx->staging.speed);

                TCHAR label[32];
                wsprintf(label, _T("Scroll Speed: %d"), ctx->staging.speed);
                SetDlgItemText(hDlg, IDC_SPEED_LABEL, label);

                // -----------------------------------------------
                // ダイアログのサイズを実ピクセル単位で確定させる
                // -----------------------------------------------
                int desiredClientH = y;

                RECT rc = { 0, 0, desiredClientW, desiredClientH };
                AdjustWindowRectEx(&rc,
                    (DWORD)GetWindowLongPtr(hDlg, GWL_STYLE),
                    FALSE,
                    (DWORD)GetWindowLongPtr(hDlg, GWL_EXSTYLE));

                int winW = rc.right - rc.left;
                int winH = rc.bottom - rc.top;

                // -----------------------------------------------
                // 表示位置を決定する
                // -----------------------------------------------
                int posX, posY;
                bool hasSavedPos = (g_cfg_dialog_pos_x.get() != DIALOG_POS_UNSET &&
                    g_cfg_dialog_pos_y.get() != DIALOG_POS_UNSET);

                if (hasSavedPos)
                {
                    posX = (int)g_cfg_dialog_pos_x.get();
                    posY = (int)g_cfg_dialog_pos_y.get();

                    POINT center = { posX + winW / 2, posY + winH / 2 };
                    HMONITOR hMon = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = { sizeof(mi) };
                    GetMonitorInfo(hMon, &mi);

                    if (posX < mi.rcWork.left) posX = mi.rcWork.left;
                    if (posY < mi.rcWork.top) posY = mi.rcWork.top;
                    if (posX + winW > mi.rcWork.right) posX = mi.rcWork.right - winW;
                    if (posY + winH > mi.rcWork.bottom) posY = mi.rcWork.bottom - winH;
                }
                else
                {
                    HWND ownerWnd = GetParent(hDlg);
                    RECT rcOwner;
                    HMONITOR hMon = NULL;
                    if (ownerWnd && GetWindowRect(ownerWnd, &rcOwner))
                        hMon = MonitorFromRect(&rcOwner, MONITOR_DEFAULTTONEAREST);
                    else
                    {
                        POINT ptZero = { 0, 0 };
                        hMon = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
                    }

                    MONITORINFO mi = { sizeof(mi) };
                    GetMonitorInfo(hMon, &mi);
                    posX = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - winW) / 2;
                    posY = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - winH) / 2;
                }
 
                UpdateTextGroupEnables(hDlg, ctx);
                EnableWindow(GetDlgItem(hDlg, IDC_HOVER_FRAME_COLOR_BTN),
                    ctx->staging.showHoverFrame && !ctx->staging.useThemeColors);

                SetWindowPos(hDlg, NULL, posX, posY, winW, winH, SWP_NOZORDER);

                return TRUE;
            }

            case WM_HSCROLL:
            {
                SettingsDialogContext* ctx = (SettingsDialogContext*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                HWND slider = GetDlgItem(hDlg, IDC_SPEED_SLIDER);
                int pos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
                if (ctx) ctx->staging.speed = pos;

                TCHAR label[32];
                wsprintf(label, _T("Scroll Speed: %d"), pos);
                SetDlgItemText(hDlg, IDC_SPEED_LABEL, label);
                return TRUE;
            }

            case WM_NOTIFY:
            {
                SettingsDialogContext* ctx = (SettingsDialogContext*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                NMHDR* nmhdr = (NMHDR*)lp;
                if (nmhdr->code == UDN_DELTAPOS)
                {
                    NMUPDOWN* nmud = (NMUPDOWN*)lp;
                    int newPos = nmud->iPos + nmud->iDelta;
                    if (newPos < 1) newPos = 1;
                    if (newPos > 10) newPos = 10;

                    if (ctx)
                    {
                        if (nmhdr->idFrom == IDC_GAP1_SPIN)
                            ctx->staging.gap1 = newPos;
                        else if (nmhdr->idFrom == IDC_GAP2_SPIN)
                            ctx->staging.gap2 = newPos;
                    }
                }
                return TRUE;
            }
           
            case WM_COMMAND:
            {
                SettingsDialogContext* ctx = (SettingsDialogContext*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

                if (LOWORD(wp) == IDC_FONT_BUTTON)
                {
                    LOGFONT lf = ctx ? ctx->staging.font : g_cfg_album_font.get();

                    CHOOSEFONT cf = {};
                    cf.lStructSize = sizeof(cf);
                    cf.hwndOwner = hDlg;
                    cf.lpLogFont = &lf;
                    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

                    if (ChooseFont(&cf) && ctx)
                        ctx->staging.font = lf;
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_THEME_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_THEME_CHECK);
                    bool checked = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (ctx) ctx->staging.useThemeColors = checked;

                    EnableWindow(GetDlgItem(hDlg, IDC_BG_COLOR_BTN), !checked && ctx && !ctx->staging.bgUseImage);
                    EnableWindow(GetDlgItem(hDlg, IDC_NOART_COLOR_BTN), !checked && ctx && !ctx->staging.noArtUseImage);
                    EnableWindow(GetDlgItem(hDlg, IDC_HOVER_FRAME_COLOR_BTN), !checked && ctx && ctx->staging.showHoverFrame);
                    UpdateTextGroupEnables(hDlg, ctx);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_ARTWORK_QUALITY_COMBO && HIWORD(wp) == CBN_SELCHANGE)
                {
                    HWND combo = GetDlgItem(hDlg, IDC_ARTWORK_QUALITY_COMBO);
                    int idx = (int)SendMessage(combo, CB_GETCURSEL, 0, 0);
                    int value = (int)SendMessage(combo, CB_GETITEMDATA, idx, 0);
                    if (ctx) ctx->staging.artworkQuality = value;
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_NOART_MODE_IMAGE)
                {
                    HWND check = GetDlgItem(hDlg, IDC_NOART_MODE_IMAGE);
                    bool checked = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (ctx) ctx->staging.noArtUseImage = checked;

                    EnableWindow(GetDlgItem(hDlg, IDC_NOART_COLOR_BTN),
                        ctx && !ctx->staging.useThemeColors && !checked);
                    EnableWindow(GetDlgItem(hDlg, IDC_NOART_IMAGE_BROWSE_BTN), checked);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_BG_MODE_IMAGE)
                {
                    HWND check = GetDlgItem(hDlg, IDC_BG_MODE_IMAGE);
                    bool checked = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (ctx) ctx->staging.bgUseImage = checked;

                    EnableWindow(GetDlgItem(hDlg, IDC_BG_COLOR_BTN),
                        ctx && !ctx->staging.useThemeColors && !checked);
                    EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_BROWSE_BTN), checked);
                    EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_QUALITY_COMBO),
                        checked && ctx && !ctx->staging.stabilityMode);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_NOART_IMAGE_BROWSE_BTN)
                {
                    TCHAR pathBuf[MAX_PATH] = {};
                    OPENFILENAME ofn = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = _T("Image Files\0*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tiff\0All Files\0*.*\0");
                    ofn.lpstrFile = pathBuf;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (GetOpenFileName(&ofn) && ctx)
                    {
                        pfc::stringcvt::string_utf8_from_os utf8Path(pathBuf);
                        ctx->staging.noArtImagePath = utf8Path.get_ptr();
                        SetWindowText(GetDlgItem(hDlg, IDC_NOART_IMAGE_EDIT), pathBuf);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_BG_IMAGE_BROWSE_BTN)
                {
                    TCHAR pathBuf[MAX_PATH] = {};
                    OPENFILENAME ofn = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = _T("Image Files\0*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tiff\0All Files\0*.*\0");
                    ofn.lpstrFile = pathBuf;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (GetOpenFileName(&ofn) && ctx)
                    {
                        pfc::stringcvt::string_utf8_from_os utf8Path(pathBuf);
                        ctx->staging.bgImagePath = utf8Path.get_ptr();
                        SetWindowText(GetDlgItem(hDlg, IDC_BG_IMAGE_EDIT), pathBuf);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_BG_IMAGE_QUALITY_COMBO && HIWORD(wp) == CBN_SELCHANGE)
                {
                    HWND combo = GetDlgItem(hDlg, IDC_BG_IMAGE_QUALITY_COMBO);
                    int idx = (int)SendMessage(combo, CB_GETCURSEL, 0, 0);
                    if (ctx) ctx->staging.bgImageQuality = idx;
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_BG_COLOR_BTN)
                {
                    static COLORREF customColors[16] = {};
                    CHOOSECOLOR cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hDlg;
                    cc.rgbResult = ctx ? ctx->staging.bgColor : g_cfg_bg_color.get();
                    cc.lpCustColors = customColors;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) && ctx)
                    {
                        ctx->staging.bgColor = cc.rgbResult;
                        if (ctx->hBgBrush) DeleteObject(ctx->hBgBrush);
                        ctx->hBgBrush = CreateSolidBrush(cc.rgbResult);
                        InvalidateRect(GetDlgItem(hDlg, IDC_BG_COLOR_SWATCH), NULL, TRUE);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_TEXT_COLOR_BTN)
                {
                    static COLORREF customColors[16] = {};
                    CHOOSECOLOR cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hDlg;
                    cc.rgbResult = ctx ? ctx->staging.textColor : g_cfg_text_color.get();
                    cc.lpCustColors = customColors;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) && ctx)
                    {
                        ctx->staging.textColor = cc.rgbResult;
                        if (ctx->hTextBrush) DeleteObject(ctx->hTextBrush);
                        ctx->hTextBrush = CreateSolidBrush(cc.rgbResult);
                        InvalidateRect(GetDlgItem(hDlg, IDC_TEXT_COLOR_SWATCH), NULL, TRUE);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_NOART_COLOR_BTN)
                {
                    static COLORREF customColors[16] = {};
                    CHOOSECOLOR cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hDlg;
                    cc.rgbResult = ctx ? ctx->staging.noArtColor : g_cfg_noart_color.get();
                    cc.lpCustColors = customColors;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) && ctx)
                    {
                        ctx->staging.noArtColor = cc.rgbResult;
                        if (ctx->hNoArtBrush) DeleteObject(ctx->hNoArtBrush);
                        ctx->hNoArtBrush = CreateSolidBrush(cc.rgbResult);
                        InvalidateRect(GetDlgItem(hDlg, IDC_NOART_COLOR_SWATCH), NULL, TRUE);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_HOVER_FRAME_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_HOVER_FRAME_CHECK);
                    bool checked = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (ctx) ctx->staging.showHoverFrame = checked;

                    EnableWindow(GetDlgItem(hDlg, IDC_HOVER_FRAME_COLOR_BTN),
                        checked && ctx && !ctx->staging.useThemeColors);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_HOVER_FRAME_COLOR_BTN)
                {
                    static COLORREF customColors[16] = {};
                    CHOOSECOLOR cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hDlg;
                    cc.rgbResult = ctx ? ctx->staging.hoverFrameColor : g_cfg_hover_frame_color.get();
                    cc.lpCustColors = customColors;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) && ctx)
                    {
                        ctx->staging.hoverFrameColor = cc.rgbResult;
                        if (ctx->hHoverFrameBrush) DeleteObject(ctx->hHoverFrameBrush);
                        ctx->hHoverFrameBrush = CreateSolidBrush(cc.rgbResult);
                        InvalidateRect(GetDlgItem(hDlg, IDC_HOVER_FRAME_COLOR_SWATCH), NULL, TRUE);
                    }
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_ADD_LINE_BTN)
                {
                    HWND check = GetDlgItem(hDlg, IDC_ADD_LINE_BTN);
                    bool checked = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (ctx) ctx->staging.useSecondLine = checked;
                    UpdateTextGroupEnables(hDlg, ctx);
                    return TRUE;
                    }
                else if (LOWORD(wp) == IDC_FIXTEXTPOS_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_FIXTEXTPOS_CHECK);
                    if (ctx) ctx->staging.fixTextPosition = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    UpdateTextGroupEnables(hDlg, ctx);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_SHOWNAME_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_SHOWNAME_CHECK);
                    if (ctx) ctx->staging.showAlbumName = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    UpdateTextGroupEnables(hDlg, ctx);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_WHEEL_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_WHEEL_CHECK);
                    if (ctx) ctx->staging.wheelScroll = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_PERSPECTIVE_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_PERSPECTIVE_CHECK);
                    if (ctx) ctx->staging.perspective = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_SHOWNOART_CHECK)
                {
                    HWND check = GetDlgItem(hDlg, IDC_SHOWNOART_CHECK);
                    if (ctx) ctx->staging.showNoArt = (SendMessage(check, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_DIR_LTR)
                {
                    if (ctx) ctx->staging.scrollRTL = false;
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_DIR_RTL)
                {
                    if (ctx) ctx->staging.scrollRTL = true;
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_MODE_CUSTOM)
                {
                    if (ctx) ctx->staging.stabilityMode = false;
                    EnableWindow(GetDlgItem(hDlg, IDC_SHOWNOART_CHECK), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_ARTWORK_QUALITY_COMBO), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_QUALITY_COMBO), ctx && ctx->staging.bgUseImage);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDC_MODE_STABILITY)
                {
                    if (ctx) ctx->staging.stabilityMode = true;

                    HWND showNoArtCheck = GetDlgItem(hDlg, IDC_SHOWNOART_CHECK);
                    SendMessage(showNoArtCheck, BM_SETCHECK, BST_CHECKED, 0);
                    EnableWindow(showNoArtCheck, FALSE);
                    if (ctx) ctx->staging.showNoArt = true;

                    HWND qualityCombo = GetDlgItem(hDlg, IDC_ARTWORK_QUALITY_COMBO);
                    SendMessage(qualityCombo, CB_SETCURSEL, 3, 0);  // Low（表示順4番目＝内部値2）
                    EnableWindow(qualityCombo, FALSE);
                    if (ctx) ctx->staging.artworkQuality = 2;  // Low

                    HWND bgQualityCombo = GetDlgItem(hDlg, IDC_BG_IMAGE_QUALITY_COMBO);
                    SendMessage(bgQualityCombo, CB_SETCURSEL, 2, 0);  // Low
                    EnableWindow(bgQualityCombo, FALSE);
                    if (ctx) ctx->staging.bgImageQuality = 2;  // Low
                    return TRUE;
                    }
                else if (LOWORD(wp) == IDC_APPLY_BTN)
                {
                    if (ctx) CommitSettings(hDlg, ctx);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDOK)
                {
                    if (ctx) CommitSettings(hDlg, ctx);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                else if (LOWORD(wp) == IDCANCEL)
                {
                    // ステージング領域を破棄するだけで、cfgには一切触れない
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                break;
            }

            case WM_DESTROY:
            {
                RECT rc;
                if (GetWindowRect(hDlg, &rc))
                {
                    g_cfg_dialog_pos_x = rc.left;
                    g_cfg_dialog_pos_y = rc.top;
                }

                SettingsDialogContext* ctx = (SettingsDialogContext*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                if (ctx)
                {
                    if (ctx->hBgBrush) DeleteObject(ctx->hBgBrush);
                    if (ctx->hTextBrush) DeleteObject(ctx->hTextBrush);
                    if (ctx->hNoArtBrush) DeleteObject(ctx->hNoArtBrush);
                    if (ctx->hHoverFrameBrush) DeleteObject(ctx->hHoverFrameBrush);
                    delete ctx;
                    SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
                }
                return TRUE;
            }

            case WM_CLOSE:
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            return FALSE;
        }

        // =======================================================
        // 設定ダイアログを動的に作成して表示する（.rcファイル不要）
        // =======================================================
        void ShowSettingsDialog(HWND parent)
        {
            // シンプルな構成のため、Win32標準のDLGTEMPLATE（非Ex版）を使う
            struct {
                DLGTEMPLATE dlg;
                WORD menu = 0;
                WORD wclass = 0;
                WCHAR title[16] = L"Settings";
            } dlgData = {};

            dlgData.dlg.style = DS_MODALFRAME | DS_SHELLFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU;
            dlgData.dlg.dwExtendedStyle = 0;
            dlgData.dlg.cdit = 0;
            dlgData.dlg.x = 0;
            dlgData.dlg.y = 0;
            dlgData.dlg.cx = 100;
            dlgData.dlg.cy = 100;

            // DialogBoxIndirectParam は標準のモーダルループを使うため、
            // 独自にメッセージループを書く必要がなく、安全に動作する
            DialogBoxIndirectParam(
                NULL, (LPCDLGTEMPLATE)&dlgData, parent,
                (DLGPROC)SettingsDialogProc, (LPARAM)this);
        }

        // =======================================================
        // 右クリックメニューの表示
        // =======================================================
        void OnContextMenu(HWND wnd, int x, int y)
        {
            POINT pt = { x, y };

            bool onArtwork = false;
            pfc::string8 hitAlbumName;
            metadb_handle_ptr hitTrack;

            if (x != -1 || y != -1)
            {
                POINT ptClient = pt;
                ScreenToClient(wnd, &ptClient);
                onArtwork = HitTestAlbum(ptClient.x, ptClient.y, hitAlbumName, hitTrack);
            }
            else
            {
                pt.x = 0;
                pt.y = 0;
                ClientToScreen(wnd, &pt);
            }

            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, 1, _T("Settings..."));
            if (onArtwork)
            {
                AppendMenu(menu, MF_SEPARATOR, 0, NULL);   // ← 追加：区切り線
                AppendMenu(menu, MF_STRING, 2, _T("Properties"));
            }

            int selected = TrackPopupMenu(menu,
                TPM_RIGHTBUTTON | TPM_RETURNCMD,
                pt.x, pt.y, 0, wnd, NULL);

            DestroyMenu(menu);

            if (selected == 1)
            {
                ShowSettingsDialog(wnd);
            }
            else if (selected == 2 && onArtwork)
            {
                metadb_handle_list album_tracks;
                CollectAlbumTracks(hitAlbumName, hitTrack, album_tracks);

                if (album_tracks.get_count() > 0)
                {
                    service_ptr_t<contextmenu_manager> api = contextmenu_manager::g_create();
                    api->init_context(album_tracks, 0);

                    contextmenu_node* propNode = FindMenuNodeByName(api->get_root(), "Properties");
                    if (propNode)
                        propNode->execute();
                }
            }
        }

        // -------------------------------------------------------
        // メンバ変数
        // -------------------------------------------------------
        HWND                                 m_hwnd = NULL;

        metadb_handle_ptr                    m_current_track;
        std::vector<LibAlbum>                m_lib_albums;
        std::map<std::string, ArtCacheEntry> m_art_cache;
        std::deque<QueueEntry>                m_queue;
        int                                   m_last_album_index = -1;
        std::mt19937                         m_rng;
        cui::colours::helper                 m_colours;
        titleformat_object::ptr               m_compiled_script1;
        pfc::string8                          m_cached_format1;
        titleformat_object::ptr               m_compiled_script2;
        pfc::string8                          m_cached_format2;
        HBITMAP                               m_bgImageBitmap = NULL;
        std::string                           m_bgImagePathLoaded;
        HBITMAP                               m_noArtImageBitmap = NULL;
        std::string                           m_noArtImagePathLoaded;

        // シングル左クリックで作成したプレイリストを、直後のダブルクリックで
        // 再利用するための記憶領域（未設定はpfc::infinite_size）
        t_size                                 m_lastClickPlaylist = pfc::infinite_size;
        pfc::string8                           m_lastClickAlbumName;

        // マウスオーバー枠表示のためのマウス位置追跡
        int                                     m_mouseX = -1;
        int                                     m_mouseY = -1;
        bool                                    m_mouseInWindow = false;
        // 直近でホバー中と判定していたエントリ（変化検知用。再描画は
        // これが変わった時だけ行う）
        QueueEntry*                             m_lastHoverEntry = nullptr;
    };
     
    // Columns UI パネルとして登録
    static uie::window_factory<AlbumTrainPanel>
        g_albumtrain_cui_factory;

} // namespace