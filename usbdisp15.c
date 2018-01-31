#include <windows.h>

#include "ftd2xx.h"

int dwidth=900;
int dheight=600;
unsigned char *bytes;

// 10 buffers
#define bufs0 30000//#define bufs0    193536
#define bufsize 300000//#define bufsize   193536*2


#define vbufsize 392165
#define vbufsized 392165*2

//int baud = 4255320;
//int baud = 4290000; // 2017.4.3 ���܂Ŏg���Ă��{�[���[�g// 2016.12.10 10:40 ����
//int baud = 4375400;    // 2017.4.3 13:10 �{�[���[�g���グ�Ă݂�
//int baud = 4414397;//    24kHz
int baud = 2363962;  //    15kHz
BYTE buf[bufsize];   //�ǂݍ��݃o�b�t�@
BYTE vbuf[vbufsize]; //�t���[�����f�[�^�؂�o���o�b�t�@
//BYTE vbuf[vbufsized];
BYTE hbuf[620][900]; //���������������t���[���o�b�t�@
BYTE bhbuf[620][900];//�P�t���[���O�̃t���[���o�b�t�@

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

//���[�h�X���b�h
DWORD WINAPI readproc(LPVOID x) {
  int i,q;
  DWORD n; // FT_Read�œǂݍ��܂ꂽ�o�C�g��
  
  BYTE *bufp[11];
  for(i=1;i<=10;i++)  bufp[i]=buf+bufs0*(i-1);
  
  while(1) {    // hvt ... handle vertical sync thread
    for(q=1; q<=10; q++) {
      //rc0=q;;// if(vi0 != q) ResumeThread(hvt);
      fts = FT_Read(hFt, bufp[q], bufs0, &n);
      EnterCriticalSection(&rcs);
      buflen0++;
      LeaveCriticalSection(&rcs);
      //if(buflen0 >0) ResumeThread(hvt);
      ResumeThread(hvt);
    }
  }
}

void vringbufinit(void) {
  vpointer=0;bvi0=0;
}

BYTE vnext0(void) {
  BYTE r0;
  bvi0=vi0;
  vi0=vpointer/bufs0+1;

  if(bvi0 != vi0) {
    //while(buflen0==0);
    if(buflen0==0) SuspendThread(hvt);
    EnterCriticalSection(&rcs);
    buflen0--;
    LeaveCriticalSection(&rcs);
  }
  r0=buf[vpointer++];
  if(vpointer > bufsize-1) {vpointer=0;}

  return r0;
}
  
//�P�t���[���؂�o���X���b�h
DWORD WINAPI vproc(LPVOID x) {
  BYTE a0,ba0;
  // �t���[������u���ɕ\�����邽�߂̃t���b�v�t���b�v�ϐ�
  int ff0;


  vringbufinit();
  vcounter=0;
  ff0=0;
  //pvbuf1=vbuf;
  //pvbuf2=vbuf+vbufsize
//  vlen=0;
  while(1) {
    ba0=a0; //�ЂƂO�̃f�[�^
    a0 = vnext0();
    //�������������Ă�����
    if((a0 & 0x20) == 0) {vbuf[vcounter]=a0;vcounter=0;continue;}
    else if((ba0 & 0x20) == 0) {//���������̗����オ��
                                // ba0(�O�̃f�[�^)��0�ŁA
                                //���ǂ񂾃f�[�^a0��0�ł͂Ȃ�
      //ResumeThread(hht);
      // �t���[������u���ɕ\��
      /*if(ff0 == 0) {
        ResumeThread(hht);
        ff0 == 1;
      }
      else {
        ff0 == 0;
      }*/
      ResumeThread(hht);
    }
    
    if(vcounter<vbufsize) vbuf[vcounter++]=a0;
  }
}

//���������̃X���b�h
DWORD WINAPI hproc(LPVOID x) {
  BYTE a0, tmp;
  int i,q;

  hcounter=pcounter=0; //  ������
  SuspendThread(hht); //  ���������~

  while(1) {
    //if(vlen == 0) SuspendThread(hht);
    a0 = vbuf[pcounter++];

    if((a0 & 0x10) == 0) {    // �������������Ă�����
      if(q==1) {  // ��������������
        wid=hcounter;
        //if(wid < 795 || wid > 814) {//if(wid != 813 && wid !=812 && wid !=814) {
        /*if(wid !=813) {
          for(i=0;i<813;i++) {
            //����������������
            hbuf[hsynccounter][i]=bhbuf[hsynccounter][i];
            //hbuf[hsynccounter][i]=0;
          }
        }*/
      }

      hcounter=0;q=0;
      continue;
    }
    else {q=1;}
    if(hsynccounter<dheight&&hcounter<dwidth) {
      //�P�t���[���O�̃o�b�t�@
      bhbuf[hsynccounter][hcounter]=hbuf[hsynccounter][hcounter];
      
      //�����������l�������t���[���o�b�t�@
      hbuf[hsynccounter][hcounter]=a0;
    }
    
    hcounter++;if(hcounter==1){hsynccounter++;}

    if((a0 & 0x20) == 0) { //����������������
      pcounter=0;

      lines=hsynccounter;
      hsynccounter=0;
      //if(lines==440) {ResumeThread(hdt);} //�L���ȃ��C�����̎��̂� 24kHz
      
      ResumeThread(hdt);
      SuspendThread(hht);
    }
  }
}

//�\���X���b�h
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
  char txt0[90];


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
    if(startflag==0) {
      //�ǂݏo���X���b�h�̏���
      buflen0=0;
      InitializeCriticalSection(&rcs);
      //InitializeCriticalSection(&vcs);
      if((hrt = CreateThread(NULL, 0, readproc, (LPVOID)0, 0, &rtid)) == NULL) {
        MessageBox(0,"hrt","",MB_OK);
        return 0;
      }
      //�t���[���؂�o���X���b�h�̏���
      if((hvt = CreateThread(NULL, 0, vproc, (LPVOID)0, 0, &vtid)) == NULL) {
        MessageBox(0,"hvt","",MB_OK);
        return 0;
      }

      //���������̃X���b�h����
      if((hht = CreateThread(NULL, 0, hproc, (LPVOID)0, 0, &htid)) == NULL) {
        MessageBox(0,"hht","",MB_OK);
        return 0;
      }

      
      ghwnd = hwnd;
      //�\���p�X���b�h����
      if((hdt = CreateThread(NULL, 0, hyouji0, (LPVOID)0, 0, &dtid)) == NULL) {
        MessageBox(0, "hdt", "",MB_OK);
        return 0;
      }
      startflag=1;

      //�X���b�h�̗D�揇�� �Ƃ肠���� Read����
      if(0 == SetThreadPriority(hrt, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"r prio","",MB_OK);
        return 0;
      }
      //�X���b�h�̗D�揇�� vproc
      if(0 == SetThreadPriority(hvt, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"v prio","",MB_OK);
        return 0;
      }
      //�X���b�h�̗D�揇�� hproc
      if(0 == SetThreadPriority(hht, THREAD_PRIORITY_TIME_CRITICAL)) {
        MessageBox(0,"h prio","",MB_OK);
        return 0;
      }
      
    }
    return 0;
  case WM_KEYDOWN:
    //�e�X�g�p�̃{�[���[�g��
    if(wp == 0x31) {baud--;}
    else if(wp == 0x32) {baud -=10;}
    else if(wp == 0x33) {baud -=100;}
    else if(wp == 0x34) {baud +=100;}
    else if(wp == 0x35) {baud +=10;}
    else if(wp == 0x36) {baud++;}
    else if(wp == 0x39) {ZeroMemory(hbuf,620*900);} // ���݂�����

    fts = FT_SetBaudRate(hFt, baud);
    if(fts != FT_OK) {MessageBox(0,"sbr", "",MB_OK);return -1;}

    return 0;

  case WM_PAINT:
    hdc = BeginPaint(hwnd , &ps);

    //BitBlt(hdc , 0 , 0 , dwidth, dheight , hBuffer , 0 , 0 , SRCCOPY);
    StretchBlt(hdc , -50 , 0 , dwidth , dheight , hBuffer , 0 , 0 ,
               dwidth/1.1,dheight/1.9,SRCCOPY);
    /* BOOL TextOut(
  HDC hdc,           // �f�o�C�X�R���e�L�X�g�̃n���h��
  int nXStart,       // �J�n�ʒu�i��_�j�� x ���W
  int nYStart,       // �J�n�ʒu�i��_�j�� y ���W
  LPCTSTR lpString,  // ������
  int cbString       // ������
);*/
    sprintf(txt0,"hcounter %08d, width %08d, lines %06d, baud %010d, buflen0=%04d", hcounter,wid, lines, baud, buflen0);
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
  winc.lpszClassName	= TEXT("USB DISP");
  
  if (!RegisterClass(&winc)) {MessageBox(0, "opn", "",MB_OK);return -1;}
  
  hwnd = CreateWindow(
    TEXT("USB DISP") , TEXT("USB DISP") ,
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
