# MiniBrow

Windows 95 でも動く、軽量の Web ブラウザです。
追加の DLL やインストールは要りません。

## 使い始める

[Releases](https://github.com/nanitaro/MiniBrow/releases) からアーカイブを取得し、フォルダの構成を変えずに好きな場所へ展開して、`minibrow.exe` を起動してください。

| ファイル | 内容 |
|---|---|
| `minibrow.exe` | 本体 |
| `minibrow.ini` | 設定ファイル |
| `README.txt` | 説明書 |
| `LICENSE.txt` | MiniBrow 本体のライセンス (MIT License) |
| `THIRD_PARTY_LICENSES.txt` | 使用しているライブラリのライセンス |
| `xs\licenses\` | LGPL・GPL などのライセンスの本文 |

HTTP のページは、このまま見られます。HTTPS のページを見るには、次の準備が必要です。

## HTTPS のページを見るための準備 (最初に 1 回だけ)

HTTPS の接続が安全かどうかを確かめるために、ルート証明書のファイル `cacert.pem` が必要です。配布物には含まれていないので、次の手順で用意してください。

1. インターネットにつながる別のパソコンのブラウザで、次のアドレスからファイルを保存します。
   https://curl.se/ca/cacert.pem
2. 保存したファイルを、フロッピーディスクや LAN などで MiniBrow を使うパソコンへ移します。
3. ファイル名を `cacert.pem` のままにして、`minibrow.exe` と同じフォルダに置きます。
4. MiniBrow を起動し直します。

一度置けば、その後は 30 日ごとに起動時に自動で更新を確認します。スタートページの「今すぐルート証明書を更新する」でも更新できます。

このファイルが無いと、次のように表示されて HTTPS のページは開けません。

> cacert.pem (ルート証明書) が見つからないか読めないため、HTTPS の証明書を検証できません。

うまくいかないときは、次の点を確かめてください。

- パソコンの日付が正しいこと (ずれていると検証に失敗します)
- ファイル名が `cacert.pem.txt` などに変わっていないこと

`minibrow.ini` の `VerifyCert=0` にすると `cacert.pem` が無くても接続できますが、偽のサイトを見分けられなくなるので、おすすめしません。

TLS 1.3 にしか対応していないサイトは表示できません。古いパソコンでは、HTTPS の接続に数秒かかることがあります。

TLS 1.0 / 1.1 は安全性が低いので、初めは使わない設定になっています。古い機器の管理画面など、TLS 1.0 / 1.1 にしか対応していないページを開くときだけ、`minibrow.ini` の `OldTls=1` にしてください。

## 操作

| キー | 動作 |
|---|---|
| Alt+← / Alt+→ / BackSpace | 戻る / 進む |
| F5 / Ctrl+R | 更新 |
| Esc | 読み込みの中止 |
| F6 / Ctrl+L / Alt+D | アドレス欄へ移動 |
| Ctrl+D | ブックマークに追加 |
| Ctrl + + / Ctrl + - / Ctrl + 0 | 文字の拡大 / 縮小 / 100% に戻す |
| Ctrl+A / Ctrl+C | すべて選択 / コピー |

JIS キーボードの「+」は「;」のキーです。テンキーの + と - も使えます。

ブックマークは、ツールバーの「ブックマーク」から開きます。一覧の整理は、メニューの「ブックマークの編集」でメモ帳を使って行います。

文字の大きさは、ページの上で右クリックして「文字の大きさ」からも選べます。

## 設定

`minibrow.ini` をメモ帳で開いて変更します。各項目の説明は、ファイルの中に書いてあります。変更は、次に起動したときに反映されます。

`minibrow.ini` が無いときは、起動したときに既定の内容で新しく作ります。設定を初めの状態に戻したいときは、`minibrow.ini` を消して起動し直してください。

よく使う項目:

| 項目 | 内容 |
|---|---|
| `Home=` | 起動時に開くページ |
| `Proxy=` | プロキシサーバー |
| `FontSize=` | 標準の文字の大きさ (px) |
| `Zoom=` | 起動時の文字の倍率 (%) |
| `ZoomPx=` | px 指定の文字も拡大・縮小するか (1 = する) |
| `Images=` | 画像を読み込むか (1 = 読み込む) |
| `JavaScript=` | JavaScript を動かすか (1 = 動かす) |
| `Decorations=` | 角の丸めや影を描くか。遅いパソコンでは 0 にすると軽くなります |

## MiniBrow が作るファイル

`minibrow.exe` と同じフォルダに、次のファイルを作ります。

| ファイル | 内容 |
|---|---|
| `cookies.txt` | Cookie |
| `visited.txt` | 訪れたリンクの記録 (URL のハッシュだけ) |
| `bookmarks.txt` | ブックマーク |
| `psl.bin` | Cookie の判定に使う一覧 (publicsuffix.org から自動で取得) |
| `storage\` | Web ページが保存したデータ (localStorage) |

MiniBrow を別の人に渡すときは、これらのファイルを含めないでください。ログイン中の情報や閲覧の記録が含まれていることがあります。

## できないこと

- 大きな最近の Web サイトは、量と速さの点でほとんど動きません
- TLS 1.3 にしか対応していないサイトは表示できません
- 動画や音声は再生できません
- 印刷はできません

## ソースからの作り方

Open Watcom C 2.0 で作ります。Open Watcom を `C:\WATCOM` に入れた場合の例です。

```powershell
$env:WATCOM="C:\WATCOM"; $env:PATH="C:\WATCOM\binnt64;C:\WATCOM\binnt;$env:PATH"; $env:INCLUDE="C:\WATCOM\h;C:\WATCOM\h\nt"
wmake -h
```

できた `minibrow.exe` が配布物の本体です。JavaScript エンジン (XS) への変更は `xs\MINIBROW-CHANGES.txt` にあります。

## ライセンス

MiniBrow 本体は MIT License で提供しています。[LICENSE.txt](LICENSE.txt) をご覧ください。

MiniBrow が使用しているライブラリのライセンスは、[THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt) と `xs\licenses\` をご覧ください。

JavaScript エンジンの Moddable XS は、GNU Lesser General Public License (LGPL) version 3 以降で提供されています。XS を変更して `minibrow.exe` を作り直せるよう、ソースコードをこのリポジトリで公開しています。
