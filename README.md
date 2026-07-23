# Album Train

A Columns UI panel component for foobar2000 (64-bit only). Inspired by the "Discovery Train" feature from Sony's "x-アプリ" ("x-appli"), it displays album artwork from your library scrolling across the panel in a horizontal row.

foobar2000（64bit版）向けの Columns UI パネルコンポーネントです。Sony「x-アプリ」の「ディスカバリートレイン」を参考に、ライブラリ内のアルバムアートワークを横一列に流れるように表示します。

## Screenshot / スクリーンショット

**Startup and scrolling / 起動〜スクロール動作**

![Album Train startup demo](screenshots/demo1.gif)

**Single-click artwork to add the album to a playlist / アートワークをシングルクリックでプレイリストにアルバムを追加**

![Album Train single-click demo](screenshots/demo3.gif)

**Double-click artwork to play / アートワークをダブルクリックで再生**

![Album Train double-click demo](screenshots/demo4.gif)

## Requirements / 動作環境

- foobar2000 v2.x (**64-bit only**. 32-bit is not supported)
- Columns UI 3.5.0 or later

- foobar2000 v2.x（**64bit版のみ**。32bit版には対応していません）
- Columns UI 3.5.0以降

## Installation / インストール方法

1. Download the latest `album_train.fb2k-component` from [Releases](../../releases)
2. Open `File → Preferences → Components` in foobar2000
3. Click `Install...` and select the downloaded `.fb2k-component` file
4. Click `OK` and restart foobar2000

After installation, add the "Album Train" panel to your layout from Columns UI's layout editing mode. Alternatively, you can add it via `Preferences → Display → Columns UI → Layout → Add child → Panels`.

1. [Releases](../../releases) から最新の `album_train.fb2k-component` をダウンロード
2. foobar2000 の `File → Preferences → Components` を開く
3. `Install...` をクリックし、ダウンロードした `.fb2k-component` ファイルを選択
4. `OK` を押し、foobar2000 を再起動

インストール後、Columns UI のレイアウト編集モードから「Album Train」パネルをレイアウトに追加してください。あるいは、`Preferences → Display → Columns UI → Layout → Add child → Panels` の操作からパネルを追加することもできます。

## Usage / 使い方

### Basic operation / 基本操作

- **Left click**: Adds all tracks from the clicked album to a new playlist (doesn't play)
- **Double-click**: Adds all tracks from the clicked album to a new playlist and plays it from the first track
- **Right click**:
  - On empty space → shows `Settings...` only
  - On artwork (or its album name text) → shows `Settings...` plus `Properties`. Clicking `Properties` opens the Properties window with all tracks of that album selected
- **Mouse wheel**: Manual scrolling (can be turned on/off in settings)

- **左クリック**：クリックしたアルバムの全曲を新規プレイリストに追加します（再生はしません）
- **ダブルクリック**：クリックしたアルバムの全曲を新規プレイリストに追加し、1曲目から再生します
- **右クリック**：
  - 何もない場所 → `Settings...`（設定ダイアログ）のみ表示
  - アートワーク上（またはそのアルバム名テキスト上）→ `Settings...` に加えて `Properties` を表示。クリックすると、そのアルバム全曲が選択された状態で Properties ウィンドウが開きます
- **マウスホイール**：手動でのスクロール（設定でオン/オフ可能）

### Settings dialog / 設定ダイアログ

Right-click → `Settings...` lets you customize appearance and behavior, organized into the following groups.

right-click → `Settings...`から、以下のグループで外観・挙動をカスタマイズできます。

| Group / グループ | Description / 内容 |
|---|---|
| **Mode** | Switch between Stability Focused (lightweight) and Customization Focused / Stability Focused（軽量・安定性重視）／ Customization Focused（カスタマイズ性重視）の切り替え |
| **Theme** | Whether to follow Columns UI's theme colors / Columns UI のテーマ色に追従するかどうか |
| **Artwork** | Display quality (Ultra (Beta)/High/Middle/Low), Hardware Acceleration (NVIDIA GPU only), whether to show a highlight frame on mouse hover, perspective effect, whether to show albums without artwork, and its color or custom image / 表示品質（Ultra (Beta)/High/Middle/Low）、Hardware Acceleration（NVIDIA製GPU限定）、マウスホバー時の枠表示有無、遠近感演出、アートワーク未登録アルバムの表示有無、その色または任意画像 |
| **Scroll** | Scroll direction, scroll speed, whether mouse wheel scrolling is allowed / 流れる方向、スクロール速度、マウスホイール操作の許可 |
| **Text** | Whether to show album name, font, color, display format (Title Formatting support), second line, fixed text position / アルバム名表示の有無、フォント、色、表示フォーマット（Title Formatting対応）、2行目表示、テキスト位置の固定 |
| **Background** | Background color or custom image, and its display quality / 背景色または任意画像、その表示品質 |

Selecting Stability Focused mode automatically forces "show albums without artwork" on and locks display quality to Low (a safety measure to reduce load).

Stability Focused モードを選択すると、アートワーク未登録アルバムを常に表示する設定と、表示品質が自動的に Low に固定されます（負荷を軽減するための安全対策です）。

If your system has an NVIDIA GPU, you can turn on "Hardware Acceleration" in the "Artwork" group. This renders album artwork via Direct2D instead of GDI/GDI+, and noticeably reduces the jitter that can appear during the perspective (scaling) effect. It's detected automatically and stays greyed out on other hardware. While it's on, the "Artwork Quality" dropdown has no effect and is greyed out.

お使いの環境がNVIDIA製GPUの場合、「Artwork」グループの「Hardware Acceleration」をオンにできます。遠近感演出（拡大縮小）中に発生する振動が体感できるレベルで軽減されます。GPUは自動検出され、それ以外の環境ではグレーアウトしたままになります。オンの間は「Artwork Quality」の選択はグレーアウトされます。

---

## FAQ

**Q. Does it work with the Default UI (without Columns UI)?**

A. Currently it's Columns UI only. Default UI support is on the roadmap.

**Q. Does it work with the 32-bit version of foobar2000?**

A. No. This component is 64-bit only.

**Q. How are albums without artwork displayed?**

A. If `Show Albums Without Artwork` in the "Artwork" group is on, they're shown as a solid color or a custom image. If off, they're excluded when the queue is built.

**Q. It's running slowly / stuttering.**

A. Setting the "Mode" group to `Stability Focused` automatically switches display quality to Low, reducing load. If you want to stay in `Customization Focused`, try setting "Artwork Quality" and "Image Quality" closer to Low.

**Q. The color change buttons are greyed out and I can't click them.**

A. While `Use Theme Colours` in the "Theme" group is on, colors automatically follow the Columns UI theme, so individual color settings are disabled.

**Q. Only some of an album's tracks have artwork, or only certain image types are detected — why isn't it showing?**

A. Only images tagged as `Front Cover` are displayed. If your embedded artwork is tagged as a different type (e.g. `Back Cover`, `Disc`, `Other`), it won't be picked up. Re-tagging the image as `Front Cover` should resolve this.

**Q. What is "Ultra (Beta)" in Artwork Quality?**

A. Experimental. The intended visual smoothness improvement has not been clearly noticeable in testing so far.

**Q. Hardware Acceleration is greyed out and I can't turn it on.**

A. It's only available on systems with an NVIDIA GPU, detected automatically. It stays disabled on other hardware.

**Q. Default UI（Columns UIを使わない標準UI）でも使えますか？**

A. 現時点では Columns UI 専用です。Default UI には今後対応する予定です。

**Q. 32bit版のfoobar2000でも使えますか？**

A. 使えません。64bit版専用のコンポーネントです。

**Q. アートワークが無いアルバムはどう表示されますか？**

A. 「Artwork」グループの `Show Albums Without Artwork` がオンの場合、単色または任意の画像で表示されます。オフの場合はキュー生成時に除外されます。

**Q. 動作が重い・カクつきます。**

A. 「Mode」グループを `Stability Focused` にすると、表示品質が自動的にLowになり負荷が下がります。`Customization Focused` のままにしたい場合は、「Artwork Quality」「Image Quality」をそれぞれLow寄りに設定してみてください。

**Q. 色の変更ボタンがグレーアウトしていて押せません。**

A. 「Theme」グループの `Use Theme Colours` がオンの間は、Columns UI のテーマ色に自動追従するため、個別の色設定は無効化されます。

**Q. アートワークがあるはずなのに表示されません。**

A. `Front Cover` として登録された画像のみを表示します。埋め込まれている画像が別の種別（`Back Cover`・`Disc`・`Other`など）として登録されている場合は表示されません。画像を `Front Cover` として登録し直すことで解決するはずです。

**Q. Artwork Qualityの「Ultra (Beta)」とは何ですか？**

A. 試験的機能です。狙いとしていた視覚的な滑らかさの向上が、体感できるほどには得られていません。

**Q. Hardware Accelerationがグレーアウトしていてオンにできません。**

A. NVIDIA製GPUを搭載した環境でのみ自動検出され、有効化できる機能です。それ以外の環境では無効のままとなります。

---

## Changelog / 更新履歴

<details>
<summary>Click to show full version history / クリックで全バージョン履歴を表示</summary>

```
v1.0.0: SDK integration; core Album Train panel with perspective effect
        SDK統合・遠近感のあるアルバムトレイン本体
v1.1.0: Auto-start on launch; wait for library initialization
        起動時の自動常駐・ライブラリ初期化待ち対応
v1.2.0: Changed to "Now Loading..." display
        「Now Loading...」表記への変更
v1.2.1: Fixed artwork gaps appearing during long-running sessions
        長時間運用時のアートワーク歯抜け不具合を修正
v1.3.1: Font follows theme; code cleanup
        フォントのテーマ追従・コード整理
v2.0.0: Implemented right-click context menu
        右クリックメニューの実装
v2.1.0: 10-step scroll speed customization (submenu)
        スクロール速度の10段階カスタマイズ（サブメニュー）
v2.2.0: Moved scroll speed to a slider UI dialog
        スクロール速度をスライダーUIのダイアログに移行
v2.3.0: Toggle for showing albums without artwork
        アートワーク未登録アルバムの表示有無切り替え
v2.3.1: Improved accuracy of unconfirmed album detection
        未確認アルバムの判定精度向上
v2.4.0: Toggle for allowing mouse wheel scrolling
        マウスホイールスクロールの許可切り替え
v2.5.0: Toggle for showing album name
        アルバム名表示の有無切り替え
v2.5.1: Adjusted artwork vertical balance when album name is hidden
        アルバム名非表示時のアートワーク上下バランス調整
v2.5.2: Fixed artwork overlap when album name is hidden
        アルバム名非表示時のアートワーク重なり修正
v2.6.0: Common handler for settings changes (OnSettingsChanged)
        設定変更の共通処理（OnSettingsChanged）
v2.7.0: Customizable Album Display Format (Title Formatting)
        Album Display Format（Title Formatting）によるカスタマイズ
v2.7.1: Fixed settings dialog modal loop issue
        設定ダイアログのモーダルループ不具合修正
v2.7.2: Convert line breaks to spaces
        改行をスペースに変換する対応
v2.7.3: Fixed UTF-8 multibyte character corruption
        UTF-8マルチバイト文字破損の修正
v2.8.0: Dedicated Album Train font client (later reverted)
        Album Train専用フォントクライアント（後に撤回）
v2.8.1: Switched font selection to dedicated UI (ChooseFont)
        フォント選択を専用UI（ChooseFont）に変更
v2.9.0: Customizable background color, text color, and no-artwork color
        背景色・テキスト色・欠落時の色のカスタマイズ
v2.9.1: Background transparency feature (currently unpublished, implemented internally)
        背景透明化機能（現在は非公開・内部実装済み）
v2.10.0: Toggle for perspective effect
         遠近感演出の有無切り替え
v2.11.0: Toggle for scroll direction (left/right)
         流れる方向（左右）の切り替え
v2.12.0: Added second line of text display
         2行目テキスト表示機能の追加
v2.12.1: Changed second line UI to a checkbox
         2行目のUIをチェックボックスに変更
v2.13.0: Spin button settings for artwork-text gap and line gap
         アートワーク-テキスト間隔・行間のスピンボタン設定
v2.13.1: Accurate font height via GetTextMetrics
         フォント高さをGetTextMetricsで正確に取得
v2.13.2: Fixed font descent clipping
         フォントのディセント部分の見切れ修正
v2.13.3: Fixed top/bottom clipping during perspective effect (part 1)
         遠近感演出時の上下見切れ修正（その1）
v2.13.4: Fixed top/bottom clipping during perspective effect (part 2, unified calculation)
         遠近感演出時の上下見切れ修正（その2・一元計算方式）
v2.14.0: Unified settings dialog font via DS_SHELLFONT
         設定ダイアログのフォントをDS_SHELLFONTで統一
v2.15.0: Consolidated submenu settings into the dialog; simplified right-click menu
         サブメニュー設定項目のダイアログ統合、右クリックメニューの簡略化
v2.15.1: Cached compiled Title Formatting scripts
         Title Formattingスクリプトのコンパイルキャッシュ化
v2.16.0: Added Stability Focused / Customization Focused modes
         安定性重視モード／カスタマイズ性重視モードの追加
v2.17.0: Grouped settings dialog; fixed clipping; resolved C4312 warning
         設定ダイアログのグループ化・見切れ修正・C4312警告解消
v2.18.0: Apply/OK/Cancel three-button design (staging area approach)
         Apply/OK/Cancelの3ボタン化（ステージング領域方式）
v2.18.1: Reordered groups (Mode first); removed extra spacing; fixed some clipping
         グループ順序変更（Mode先頭）・余白解消・一部見切れ修正
v2.18.2: Fixed clipping across all fonts; remember dialog position
         フォント全体の見切れ修正・ダイアログ表示位置の記憶
v2.18.3: Default panel font now matches Columns UI's "Common (list items)" font on first run
         パネル本体フォントの初回デフォルトをCommon (list items)に合わせる
v2.19.0: Split out Colours group; merged Scroll group; reordered groups
         Coloursグループ独立・Scrollグループ統合・グループ順序変更
v2.19.1: Standardized "Colors/Color" spelling to "Colours/Colour"
         Colors/Color表記をColours/Colourに統一
v2.20.0: Grey out color buttons when Use Theme Colours is on
         Use Theme Colours ON時の色ボタングレーアウト
v2.21.0: Reordered items within Album Name Display; grey out Line Gap when applicable
         Album Name Display内の並び替え・Line Gapのグレーアウト
v2.22.0: Reordered Mode group buttons; made lightweight mode the default
         Modeグループのボタン順序変更・軽量モードのデフォルト化
v2.23.0: Introduced color preview swatches
         色プレビュー（スウォッチ）の導入
v2.23.1: Fixed label clipping; fixed swatch drawing via subclassing
         ラベル見切れ修正・スウォッチ描画のサブクラス化修正
v2.23.2: Introduced reference-counted cache management (memory usage improvement)
         参照カウント方式のキャッシュ管理導入（メモリ使用量対策）
v2.23.3: Investigated smoother perspective effect (kept only the single text-width rounding fix, reverted the rest)
         遠近感演出の滑らか化検証（テキスト表示幅の一括丸め処理のみ採用、他は既存方式に巻き戻し）
v2.23.4: Fixed click-detection logic (now matches draw position, perspective scale, and overlap order)
         クリック判定ロジックの修正（描画位置・遠近スケール・重なり順への対応）
v2.23.5: Strengthened queue consistency right after wheel scrolling
         ホイールスクロール直後のキュー整合性強化
v2.23.6: Clean up artwork cache when the library is rebuilt
         ライブラリ再構築時のアートワークキャッシュ掃除
v3.0.0: Added "Properties" item on right-clicking artwork (opens Properties with all tracks of that album selected)
        アートワーク右クリック時の「Properties」項目追加（アルバム全曲選択状態でPropertiesウィンドウを開く）
v3.0.1: Added separator line between Settings... and Properties
        Settings...とPropertiesの間への区切り線追加
v3.0.2: Introduced "Theme", "Artwork", and "Background" groups; moved items into them
        「Theme」「Artwork」「Background」グループの新設・項目移設
v3.0.3: Removed "Display" and "Colours" groups; renamed "Album Name Display" to "Text"; reordered groups
        「Display」「Colours」グループの解体・「Album Name Display」の「Text」への改名・グループ順序変更
v3.1.0: Custom image support for background and no-artwork display (aspect-ratio preserved; exclusive with solid color)
        背景・No-Artwork時の任意画像表示機能（アスペクト比維持・単色塗りつぶしと排他選択）
v3.1.1: Removed "Use Colour" radio button; consolidated into a single "Use Image" checkbox
        「Use Colour」ラジオボタンを廃止し「Use Image」チェックボックスに統合
v3.1.2: Two-column settings dialog layout; moved the three buttons to the bottom-right
        設定ダイアログの2列レイアウト化・3ボタンの右下配置
v3.1.3: Reordered items within the Artwork/Text/Scroll/Background groups
        Artwork/Text/Scroll/Backgroundグループ内の項目順変更
v3.1.4: Reworded labels in the "Text" group (for more natural, symmetrical English)
        「Text」グループのラベル文言変更（英語表現の対称性を考慮）
v3.2.0: Added "Fix Text Position" option; unified grey-out conditions in the Text group
        テキスト位置のパネル下方固定オプション（Fix Text Position）、Textグループのグレーアウト条件統合
v3.3.0: Added Artwork Quality selection (High/Middle/Low); forced Low under Stability Focused
        アートワーク表示品質の選択機能（Artwork Quality：High/Middle/Low）、Stability Focused時の強制Low
v3.3.1: Added a separate Image Quality setting for the background image, independent of Artwork Quality
        背景画像専用の品質選択機能（Image Quality）をArtwork Qualityとは独立して追加
v3.3.2: Fixed layout overlap in the Background group; grey out linked to Use Image
        Backgroundグループのレイアウト重なり修正・Use Image連動グレーアウト
v3.3.3: Widened the settings dialog and groups to fix "Browse..." text clipping
        設定ダイアログ・グループ幅拡張による「Browse...」テキスト見切れの解消
v3.3.4: Extended the click hit area (artwork top through the bottom of the last text line now counts as one continuous region)
        クリック判定範囲の拡張（アートワーク上端〜最後のテキスト行下端までを1つの連続領域として判定）
v3.4.0: Split left-click into single (add to playlist only) and double (add and play the first track)
        左クリックのシングル/ダブル分離（シングル：プレイリスト追加のみ／ダブル：追加＋1曲目再生）
v3.4.1: Fixed a momentary stutter in the Album Train's scrolling on click (per-album track lists are now pre-built and sorted, removing the need to scan the whole library on every click)
        クリック時にアルバムトレインの流れが一瞬停止する現象を解消（アルバムごとの全曲リストを事前構築・ソートし、クリック時のライブラリ全走査を排除）
v3.5.0: Added a hover highlight frame around artwork on mouse-over (follows Use Theme Colours, always available regardless of Stability Focused mode)
        マウスオーバー時にアートワークへ枠を表示する機能を追加（Use Theme Coloursに統合、Stability Focusedモードでも常時有効）
v3.5.1: Fixed a momentary stutter in the Album Train's scrolling while hovering the mouse (now only redraws when the hovered entry actually changes)
        マウスホバー使用時にアルバムトレインの流れが一瞬停止する現象を解消（ホバー対象が実際に切り替わった時だけ再描画するよう変更）
v3.5.2: Fixed "Show Albums Without Artwork" not staying off after Apply/OK (a click handler that should have updated the staging value was missing)
        「Show Albums Without Artwork」チェックボックスのオン/オフがApply/OK後も保持されなかった不具合を修正（クリック時にステージング領域へ反映するハンドラが欠落していたため）
v3.5.3: Fixed albums with an embedded Front Cover sometimes showing grey (non-standard metadata in the image was causing WIC's metadata caching to fail the whole decode; switched to not loading metadata, which this component never used anyway)
        Front Coverが埋め込まれているのにグレー表示される不具合を修正（画像内の規格外メタデータがWICのメタデータキャッシュ読み込み時に巻き添えでデコード失敗を起こしていたため、メタデータを読み込まない方式に変更）
v4.0.0: Added an experimental "Ultra (Beta)" Artwork Quality option using GDI+ high-quality interpolation and a pre-scaled cache, aimed at smoother perspective scaling. Also capped the decode resolution of embedded artwork (512px) to avoid heavy albums slowing things down
        遠近感演出をより滑らかにする狙いで、GDI+高品質補間と事前縮小キャッシュを使った試験的な「Ultra (Beta)」品質を追加。あわせて、重いアルバムの原因になっていた埋め込みアートワークのデコード解像度上限（512px）を追加
v4.1.0: Added Hardware Acceleration (NVIDIA GPU only), rendering artwork via Direct2D (GDI interop, ID2D1DCRenderTarget) instead of GDI/GDI+, noticeably reducing jitter during the perspective effect. Greys out Artwork Quality while enabled. Also unified the hover-frame drawing precision with the active rendering engine (D2D/GDI+/GDI), fixing a frame-only jitter that had been present under Ultra quality
        Hardware Acceleration（NVIDIA製GPU限定）を追加。GDI相互運用のDirect2D（ID2D1DCRenderTarget）でアートワークを描画するようになり、遠近感演出時の振動が体感できるレベルで軽減。有効時はArtwork Qualityをグレーアウト。あわせて、マウスオーバー枠の描画精度を実際に使われている描画エンジン（D2D／GDI+／GDI）に合わせて統一し、Ultra品質でのみ発生していた枠だけの振動を修正
v4.1.1: Album Train now automatically reloads its contents when the monitored library folders change (files added/removed), without requiring a foobar2000 restart. Rapid consecutive changes are debounced so only one reload happens after things settle
        ライブラリの監視フォルダが変更された際（ファイルの追加・削除）、foobar2000の再起動を挟まずアルバムトレインの表示内容を自動的に再読み込みするように。連続した変更はデバウンスされ、変更が落ち着いた後に1回だけ再読み込みが行われる
v4.1.2: Improved the settings dialog's feel: buttons are now ordered OK/Cancel/Apply (matching foobar2000's own Preferences dialog); Apply stays greyed out until something actually changes, and greys out again if you revert to the original value; pressing OK with no changes no longer reloads Album Train's contents. Also, changes are now applied without reshuffling the train except for the few settings that genuinely require it (Mode, Show Albums Without Artwork, Artwork Quality, Hardware Acceleration)
        設定ダイアログの操作感を改善：ボタンの並びをOK・Cancel・Apply（foobar2000本体のPreferencesダイアログに合わせた順）に変更。実際に何か変更するまでApplyはグレーアウトしたままになり、値を元に戻すと再びグレーアウトする。何も変更せずにOKを押してもアルバムトレインの表示内容は再読み込みされない。あわせて、本当に再読み込みが必要な項目（Mode・Show Albums Without Artwork・Artwork Quality・Hardware Acceleration）以外は、表示のリシャッフルをせずに変更が反映されるように
```

</details>

---

## Roadmap / 今後の予定

### UI/UX
- Dropdown-based font selection UI
- Consider adding more steps to the Scroll Speed slider
- Ongoing settings dialog design improvements (an evolving effort, revisited as development continues)

### Performance / Architecture
- Investigate a smoother rendering method for the perspective (scaling) effect (started in v4.0.0: tried GDI+ high-quality interpolation, removing 2px rounding, a higher-frequency timer, and a mipmap-like pre-scaled cache — none produced a dramatic improvement. The remaining jitter is likely an inherent limitation of per-frame independent resampling at continuously-changing sub-pixel positions, which a simple GDI/GDI+-based renderer can't easily fix structurally. Addressed in v4.1.0 with Hardware Acceleration (NVIDIA GPU only) via Direct2D GDI interop, which noticeably reduced the jitter. Its interpolation is limited to Linear/Nearest, though, so achieving Ultra-level or better interpolation quality would require a further move to a full `ID2D1DeviceContext`/Direct3D 11 setup)
- Expand Hardware Acceleration (v4.1.0) to cover more than just artwork (e.g. the background image), which currently isn't rendered via Direct2D

### Broader UI support
- Support for foobar2000's Default UI

### Known limitations / bugs
- Background transparency can't be verified on 64-bit foobar2000 since Panel Stack Splitter isn't available there (implemented internally, not exposed)
- Regarding the "Ultra (Beta)" Artwork Quality option: as of v4.0.0 it hasn't produced a clearly noticeable visual smoothness improvement. If further testing doesn't change that, consider removing it from the settings dialog

### Under consideration (undecided)
- Font size range (tied to panel height)
- Adjust the mouse-hover frame (v3.5.0) thickness to scale somewhat with the panel/artwork size, instead of staying a fixed 2px
- Expanding the kinds of content that can be shown as a background, such as a visualizer

### UI/UX関連
- フォント選択UIのプルダウン化
- Scroll Speedのスライダをさらに多段階にすることを検討
- 設定ダイアログのデザイン改善（今後の開発を通じて随時見直していく継続的な方針）

### パフォーマンス・アーキテクチャ関連
- 遠近感演出（拡大縮小）のさらなる滑らかな表示方式の検討（v4.0.0で着手。GDI+高品質補間・2px丸めの除去・タイマー間隔の高頻度化・ミップマップ的な事前縮小キャッシュを試したが、いずれも劇的な効果は得られず。残る振動は、位置（サブピクセル単位）の連続的な変化に対する、フレームごとに独立した再サンプリングそのものの限界である可能性が高く、GDI/GDI+ベースの単純な描画方式では構造的な対応が難しいと判断。v4.1.0にて、Direct2D（GDI相互運用）によるHardware Acceleration（NVIDIA製GPU限定）で対応し、体感できるレベルで振動を軽減。ただし補間モードがLinear/Nearestに限定されるため、Ultra以上の補間品質が必要になった場合は、`ID2D1DeviceContext`／Direct3D 11によるフル構成への移行が別途必要）
- Hardware Acceleration（v4.1.0）の適用範囲拡大。現状はアートワーク本体のみDirect2D描画で、背景画像等は対象外

### 対応UI拡大
- foobar2000のDefault UIへの対応

### 既知の制約・バグ
- 背景透明化機能は64bit版foobar2000でPanel Stack Splitterが使えず検証不可（内部実装済み・非公開）
- Artwork Qualityの「Ultra (Beta)」について、v4.0.0時点では視覚的な滑らかさの向上が体感できるレベルには至っていない。今後さらに検証しても見込みが薄いようであれば、設定ダイアログの選択肢から外すことを検討する

### 今後の検討事項（未確定）
- フォントサイズ変更の許容範囲（パネル高さとの連動）
- マウスオーバー時の枠（v3.5.0）の太さを、固定2pxのままにせず、パネル・アートワークのサイズにある程度追従させる案
- ヴィジュアライザーなど、背景に表示できるコンテンツの種類を増やす構想

---

## License / ライセンス

This project is released under the [MIT License](LICENSE).

このプロジェクトは [MIT License](LICENSE) の下で公開されています。

## Development / 開発について

This component is developed using foobar2000 SDK 2025-03-07, Columns UI SDK 3.5.0, and WTL (Windows Template Library). None of these are included in this repository, as they are third-party works; you'll need to download them separately to build the project.

To build:

1. Clone this repository
2. Download the foobar2000 SDK and Columns UI SDK, and place them in an `SDK-2025-03-07` folder at the repository root (sibling to the `Album Train` folder)
3. Download WTL, and place it in a `WTL10_01_Release` folder at the repository root (sibling to the `Album Train` folder), so that its headers end up at `WTL10_01_Release/Include`
4. Open `Album Train.slnx` and build

本コンポーネントは foobar2000 SDK 2025-03-07、Columns UI SDK 3.5.0、および WTL（Windows Template Library）を用いて開発されています。いずれも第三者の著作物であるため本リポジトリには含まれておらず、ビルドには別途ダウンロードする必要があります。

ビルド手順：

1. このリポジトリをクローンする
2. foobar2000 SDKとColumns UI SDKをダウンロードし、リポジトリ直下（`Album Train`フォルダと同じ階層）に`SDK-2025-03-07`フォルダとして配置する
3. WTLをダウンロードし、リポジトリ直下（`Album Train`フォルダと同じ階層）に`WTL10_01_Release`フォルダとして配置する（ヘッダーが`WTL10_01_Release/Include`に来る形にする）
4. `Album Train.slnx`を開いてビルドする
