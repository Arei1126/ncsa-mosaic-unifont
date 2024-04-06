NCSA Mosaic Curl
===========

![GitHub viewed with Mosaic](http://github.com/downloads/alandipert/ncsa-mosaic/github.png "GitHub with Mosaic")

This is NCSA Mosaic 2.7, one of the first graphical web browsers.
If you're on Ubuntu or something like it, your time machine is fueled
up and ready to go.  Follow the instructions below to build and run.

Many thanks to [Sean MacLennan and Alan Wylie](https://web.archive.org/web/20120915154245/seanm.ca/mosaic/) for doing the heavy lifting.  And, of course, hats off to Marc Andreessen, Eric Bina, and the rest of the [NCSA](http://www.ncsa.illinois.edu/) team for kicking things off for us.  Thanks!

Building
--------

* First, install these packages:

      sudo apt-get install build-essential libmotif-dev libjpeg62-dev libpng12-dev x11proto-print-dev libxmu-headers libxpm-dev libxmu-dev libcurl-dev libfreeimage-dev

* Next, build with:

      make linux

* Run!

      src/Mosaic
 

---------
* NCSA Mosaic with Japanese(UTF-8) Input/Output support.
* Add initializing process (XtSetLanguageProc() ) to handle input methods.
* Fixed some duplicated variable declaration.
* I use curl for downloading all contents(html, image file) from www.
* I use FreeImage library to display IMG.

* インプットメソッドを使った日本語入力ができるように、xの初期化プロセスを追加した。
* 変数宣言がヘッダファイルと重複していたものを修正した。
* curlを使って、ドキュメントや画像を取得するようにした
* FreeImageライブラリで画像を表示するようにした
