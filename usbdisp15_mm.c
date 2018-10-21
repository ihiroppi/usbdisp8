#include <windows.h>

#include "ftd2xx.h"

// 2018.9.2 Sun. 15:46 Cloudy after Sunny my home
// VSのアップデートがあったので試しに動かしてみた。
// で、ボーレートを上げたり、画面の大きさ（ストレッチブリットの倍率）を
// 変えたりした。
// 結局微妙だったので画面倍率の調整ができるようにした。
// E R キーで幅の倍率を±0.01
// D F キーで高さの倍率を±0.01
// にできるようにした。

// 2018.7.23 Mon. 1:05 Sunny my home
// いったん縮小してから拡大して表示したらうまくぼけるかと
// 思ったら、そう甘くなかった…
// hmidbufferとかのコメントアウトはそのときのゴミ

int dwidth=900;
int dheight=600;
double ddw = 1.175;   // 画面の幅・高さ調整用変数 2018.9.2
double ddh = 2.2;

unsigned char *bytes;

// 10 buffers
//#define bufs0 10000 //bufs0 30000//#define bufs0    193536
//#define bufsize 100000 //bufsize 300000//#define bufsize   193536*2
#define bufs0 5000 //bufs0 30000//#define bufs0    193536
#define bufsize 50000 //bufsize 300000//#define bufsize   193536*2


#define vbufsize 392165
#define vbufsized 392165*2

//int baud = 4255320;
//int baud = 4290000; // 2017.4.3 今まで使ってたボーレート// 2016.12.10 10:40 試し
//int baud = 4375400;    // 2017.4.3 13:10 ボーレートを上げてみた
int baud = 2700000; //2363962;    // 2017.4.3 13:28 さらに上げて微調整してみた
//int baud = 3243580; // 2018.9.2 test
//int baud = 3363080;
//int baud = 3908795;

BYTE buf[bufsize];   //読み込みバッファ
BYTE vbuf[vbufsize]; //フレーム生データ切り出しバッファ
//BYTE vbuf[vbufsized];
BYTE hbuf[620][900]; //水平も処理したフレームバッファ
BYTE bhbuf[620][900];//１フレーム前のフレームバッファ

FT_STATUS fts;
FT_HANDLE hFt=0;

HDC hdc;
PAINTSTRUCT ps;
BITMAPINFOHEADER bh;
BITMAPINFO bi;
static HBITMAP hBitmap;
static HDC hBuffer;

//int i,j,k;
int hcounter, vcounter, pcounter;
int hsynccounter,lines;
int wid;
int vpointer;

HANDLE hrt,hdt,hvt,hht;
DWORD rtid,htid,vtid,dtid;
HWND ghwnd;
int startflag=0;

int buflen0;

BYTE a;
int rc0,vi0,bvi0,vi1,bvi1;
int sbs;

static CRITICAL_SECTION rcs;

void vringbufinit(void);
BYTE vnext0(void);


void ftsok(FT_STATUS b) {
  if(b != FT_OK) {MessageBox(0, "read fail", "",MB_OK);}
}

//リードスレッド
DWORD WINAPI readproc(LPVOID x) {
  int i,q;
  DWORD n; // FT_Readで読み込まれたバイト数
  
  BYTE *bufp[11];
  for(i=1;i<=10;i++)  bufp[i]=buf+bufs0*(i-1);
  
  while(1) {    // hvt ... handle vertical sync thread
    for(q=1; q<=10; q++) {
      //rc0=q;;// if(vi0 != q) ResumeThread(hvt);
      fts = FT_Read(hFt, bufp[q], bufs0, &n);
      EnterCriticalSection(&rcs);
      buflen0++;
      LeaveCriticalSection(&rcs);
      if(buflen0 >0) ResumeThread(hvt);
      //ResumeThread(hvt);
    }
  }
}

void vringbufinit(void) {
  vpointer=0;bvi0=0;
}

BYTE vnext0(void) {
  BYTE r0;
  static int q=0,qq=0;
  
  bvi0=vi0;
  vi0=vpointer/bufs0+1;

  if(bvi0 != vi0) {
    //while(buflen0==0);
    if(buflen0<=0) SuspendThread(hvt);
    EnterCriticalSection(&rcs);
    buflen0--;
    LeaveCriticalSection(&rcs);
  }
  r0=buf[vpointer++];
  if((r0 & 0x20)==0) {
    if(q==0) {
      lines=hsynccounter;      
    }
    hsynccounter=0;
    q=1;
  }
  else {
    if(q==1) {
      //if(lines == 258) ResumeThread(hdt);
      ResumeThread(hdt);
    }
    q=0;
  }
  if((r0 & 0x10)==0) {
    if(qq==0) {
      wid=hcounter;
    }
    hcounter=0;
    qq=1;
  }
  else {
    if(qq==1) {
      hsynccounter++;
    }
    qq=0;
    hcounter++;
  }
    
  if(vpointer > bufsize-1) {vpointer=0;}

  return r0;
}
  
//１フレーム切り出しスレッド
DWORD WINAPI vproc(LPVOID x) {
  register BYTE a0;//,ba0;
  // フレームを一つ置きに表示するためのフリップフロップ変数
  int ff0;

  vringbufinit();
  vcounter=0;
  ff0=0;
  //pvbuf1=vbuf;
  //pvbuf2=vbuf+vbufsize
//  vlen=0;
  hsynccounter=0;
  hcounter=0;
  
  while(1) {
    //ba0=a0; //ひとつ前のデータ
    a0 = vnext0();
    if(hsynccounter<dheight&&hcounter<dwidth) {
      hbuf[hsynccounter][hcounter]=a0;
    }
    //if(vcounter<vbufsize) vbuf[vcounter++]=a0;
  }
}


//表示スレッド
DWORD WINAPI hyouji0(LPVOID x) {
  int ix,iy,i,j,k,q,p;

  BYTE a;
  while(1) {
    SuspendThread(hdt);
    k=0;
    for(j=0;j<dheight;j++) {
      for(i=0;i<dwidth;i++) {
        a=hbuf[dheight-j][i];
        if(k+2 < sbs) {
          bytes[k]=bytes[k+1]=bytes[k+2]=0;
          if((a & 1) == 1) bytes[k+2] = 0xff;
          //bytes[k+1]=0;
          if((a & 2) == 2) bytes[k+1] = 0xff;
          //bytes[k]=0;
          if((a & 4) == 4) bytes[k] = 0xff;
          k+=3;
        }
      }
    }

    SetDIBits(hBuffer, hBitmap, 0, dheight, (VOID *)bytes, &bi, DIB_RGB_COLORS);
    InvalidateRect(ghwnd , NULL , FALSE);
  }

  return;
}


LRESULT CALLBACK WndProc(HWND hwnd , UINT msg , WPARAM wp , LPARAM lp) {
  BOOL ts;
  char txt0[128];


  switch (msg) {
  case WM_DESTROY:

    CloseHandle(hrt);
    CloseHandle(hvt);
    CloseHandle(hht);

    DeleteDC(hBuffer);
    DeleteObject(hBitmap);
    
    PostQuitMessage(0);
    return 0;
  case WM_CREATE:
    hdc = GetDC(hwnd);
    hBitmap = CreateCompatibleBitmap(hdc, dwidth, dheight);
    hBuffer = CreateCompatibleDC(hdc);
    
    SelectObject(hBuffer , hBitmap);
    SelectObject(hBuffer , GetStockObject(NULL_PEN));

    
    bh.biSize = sizeof(BITMAPINFOHEADER);
    bh.biWidth = dwidth;
    bh.biHeight = dheight;
    bh.biPlanes = 1;
    bh.biBitCount = 24;
    bh.biCompression = BI_RGB;
    bh.biSizeImage = 0;
    bh.biXPelsPerMeter = 0;
    bh.biYPelsPerMeter = 0;
    bh.biClrUsed = 0;
    bh.biClrImportant = 0;
    
    bi.bmiHeader = bh;
    bi.bmiColors[0].rgbBlue = 0;
    bi.bmiColors[0].rgbGreen = 0;
    bi.bmiColors[0].rgbRed = 0;
    bi.bmiColors[0].rgbReserved = 0;
    ReleaseDC(hwnd , hdc);

    
    
    return 0;
  case WM_LBUTTONDOWN:
    //MessageBox(0,"qqqq","",MB_OK);
    if(startflag==0) {
      //読み出しスレッドの準備
      buflen0=0;
      InitializeCriticalSection(&rcs);
      //InitializeCriticalSection(&vcs);
      if((hrt = CreateThread(NULL, 0, readproc, (LPVOID)0, 0, &rtid)) == NULL) {
        MessageBox(0,"hrt","",MB_OK);
        return 0;
      }
      //フレーム切り出しスレッドの準備
      if((hvt = CreateThread(NULL, 0, vproc, (LPVOID)0, 0, &vtid)) == NULL) {
        MessageBox(0,"hvt","",MB_OK);
        return 0;
      }

      //水平処理のスレッド準備
/*      if((hht = CreateThread(NULL, 0, hproc, (LPVOID)0, 0, &htid)) == NULL) {
        MessageBox(0,"hht","",MB_OK);
        return 0;
      }
*/
  
      
      ghwnd = hwnd;
      //表示用スレッド準備
      if((hdt = CreateThread(NULL, 0, hyouji0, (LPVOID)0, 0, &dtid)) == NULL) {
        MessageBox(0, "hdt", "",MB_OK);
        return 0;
      }
      startflag=1;

      //スレッドの優先順位 とりあえず Readだけ
      if(0 == SetThreadPriority(hrt, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"r prio","",MB_OK);
        return 0;
      }
      //スレッドの優先順位 vproc
      if(0 == SetThreadPriority(hvt, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"v prio","",MB_OK);
        return 0;
      }
      //スレッドの優先順位 hproc
      /*if(0 == SetThreadPriority(hht, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"h prio","",MB_OK);
        return 0;
      }*/
      
    }
    return 0;
  case WM_KEYDOWN:
    //テスト用のボーレート可変
    if(wp == 0x31) {baud--;}
    else if(wp == 0x32) {baud -=10;}
    else if(wp == 0x33) {baud -=100;}
    else if(wp == 0x34) {baud -=1000;}
    else if(wp == 0x35) {baud +=1000;}
    else if(wp == 0x36) {baud +=100;}
    else if(wp == 0x37) {baud +=10;}
    else if(wp == 0x38) {baud++;}
    else if(wp == 0x39) {ZeroMemory(hbuf,620*900);} // ごみを消す
    else if(wp == 0x57) { ddw -=0.0001; }  // 'W'
    else if(wp == 0x45) { ddw -=0.001; }  // 'E'           以降 画面の高さ・幅の調整
    else if(wp == 0x52) { ddw +=0.001; }  // 'R'
    else if(wp == 0x54) { ddw +=0.0001; }  // 'T'
    else if(wp == 0x53) { ddw -=0.0001; }  // 'S'
    else if(wp == 0x44) { ddh -=0.001; }  // 'D'
    else if(wp == 0x46) { ddh +=0.001; }  // 'F'
    else if(wp == 0x47) { ddw +=0.0001; }  // 'G'
    
    fts = FT_SetBaudRate(hFt, baud);
    if(fts != FT_OK) {MessageBox(0,"sbr", "",MB_OK);return -1;}

    return 0;

  case WM_PAINT:
    hdc = BeginPaint(hwnd , &ps);

    //StretchBlt(hdc , -50 , 0 , dwidth, dheight , hBuffer , 0 , 0 ,   // 2018.9.2 orig
    //           dwidth/1.1,dheight/1.9,SRCCOPY);
    StretchBlt(hdc , -50 , 0 , dwidth, dheight , hBuffer , 0 , 0 ,
               dwidth/ddw,dheight/ddh,SRCCOPY);
   /* BOOL TextOut(
  HDC hdc,           // デバイスコンテキストのハンドル
  int nXStart,       // 開始位置（基準点）の x 座標
  int nYStart,       // 開始位置（基準点）の y 座標
  LPCTSTR lpString,  // 文字列
  int cbString       // 文字数
);*/
    sprintf(txt0,"hcounter %08d, width %08d, lines %06d, baud %010d, buflen0=%04d, ddw=%f,ddh=%f",
            hcounter,wid, lines, baud, buflen0,ddw,ddh);
    TextOut(hdc, 0,0,txt0,strlen(txt0));

    EndPaint(hwnd , &ps);
    return 0;
  }
  return DefWindowProc(hwnd , msg , wp , lp);
}

int WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPrevInstance ,
                   PSTR lpCmdLine , int nCmdShow ) {
  HWND hwnd;
  MSG msg;
  WNDCLASS winc;
  
  winc.style		= CS_HREDRAW | CS_VREDRAW;
  winc.lpfnWndProc	= WndProc;
  winc.cbClsExtra	= winc.cbWndExtra	= 0;
  winc.hInstance		= hInstance;
  winc.hIcon		= LoadIcon(NULL , IDI_APPLICATION);
  winc.hCursor		= LoadCursor(NULL , IDC_ARROW);
  winc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
  winc.lpszMenuName	= NULL;
  winc.lpszClassName	= TEXT("USBDISP");
  
  if (!RegisterClass(&winc)) {MessageBox(0, "opn", "",MB_OK);return -1;}
  
  hwnd = CreateWindow(
    TEXT("USBDISP") , TEXT("USB DISP") ,
    WS_OVERLAPPEDWINDOW | WS_VISIBLE ,
    CW_USEDEFAULT , CW_USEDEFAULT ,
    dwidth,dheight,//dwidth , dheight ,
    NULL , NULL ,
    hInstance , NULL
    );

  if (hwnd == NULL) {MessageBox(0, "opnw", "",MB_OK);return -1;}
  sbs=2*dwidth*2*dheight*3;
  bytes=(BYTE *)malloc(sbs);
  fts = FT_Open(0, &hFt);
  if(fts != FT_OK) {MessageBox(0, "opn", "",MB_OK);return -1;}
  fts = FT_SetBitMode(hFt, 0x00, 0x1);
  if(fts != FT_OK) {MessageBox(0, "sbm", "",MB_OK);return -1;}
  
  fts = FT_SetBaudRate(hFt, baud);
  if(fts != FT_OK) {MessageBox(0,"sbr", "",MB_OK);return -1;}

  
  while(GetMessage(&msg , NULL , 0 , 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  fts = FT_Close(hFt);
  return msg.wParam;
}
