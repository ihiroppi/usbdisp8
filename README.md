# usbdisp8
USB DISPLAY for 8bit 8color RGBHV

FT232HLのモジュールAE-FT232HLを用いて8bitパソコンの8色 RGBHVな信号を直接読んで
映像を表示しようというものです。

なお、AE-FT232HLの結線は下記のような前提で組んであります

J2-7 AD0 ... R  
J2-8 AD1 ... G  
J2-9 AD2 ... B  

J2-11 AD4 ... H sync  
J2-12 AD5 ... V sync  



clがついてるftのlibファイルはclでのコンパイル用にコピー・リネームしたものでftのドライバに入ってるものと
同一です。逆に普通の名前のlibはbcc用になんか変換したやつです

cmp0.batがbcc32でのコンパイルバッチファイル
cmp1.batがclでのコンパイルバッチファイルです



・2018.10.21
キーアサインなど変更しました
試しなどの関係で別のファイルになっています
mmのついたソースと実行ファイルは対で、この機能を
つけてあります。

1 ... 1下げる  
2 ... 10下げる  
3 ... 100下げる  
4 ... 1000下げる  
5 ... 1000上げる  
6 ... 100上げる  
7 ... 10上げる  
8 ... 1上げる

9 ... ゴミを消す（強制再描画）

以下は画面の幅と高さの拡大率を変えるものです
w,e,r,tは横幅・s,d,f,gは高さを変えられます。
w ... -0.0001  
e ... -0.001  
r ... +0.001  
t ... +0.0001  

s ... -0.0001  
d ... -0.001  
f ... +0.001  
g ... +0.0001  

読み取りと表示幅で同期具合を変えられるので
手動で調整するとまあまあなところまでは調整できるようになりました。


それ以前のバージョンは以下のままです。

キーボードの1〜6・9が調整などに使えるようになってます
これらは開発用のもので特に変えてやってみる以外の使い道はありません
1〜6はボーレートの上げ下げで  
1 ... 1下げる  
2 ... 10下げる  
3 ... 100下げる  
4 ... 100上げる  
5 ... 10上げる  
6 ... 1上げる  
です  
9はごみを消すために強制的に再描画するためのものです
