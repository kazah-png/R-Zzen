#pragma once
#include <iomanip>
#include <map>
namespace ui {
double scale=1;std::map<int,HFONT> fonts;
const COLORREF navy=RGB(12,23,40),bg=RGB(245,247,250),white=RGB(255,255,255),ink=RGB(23,37,55),muted=RGB(98,113,132),border=RGB(223,230,237),accent=RGB(0,130,122),mint=RGB(224,247,241);
int px(double v){return int(v*scale+.5);}
HFONT face(int size,bool bold=false){int key=size+(bold?100:0);if(!fonts.count(key))fonts[key]=CreateFontW(-px(size),0,0,0,bold?FW_SEMIBOLD:FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");return fonts[key];}
void text(HDC dc,const std::wstring&s,int x,int y,int w,int h,int size=14,COLORREF color=ink,bool bold=false,UINT flags=DT_LEFT|DT_WORDBREAK){auto old=SelectObject(dc,face(size,bold));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);RECT r{px(x),px(y),px(x+w),px(y+h)};DrawTextW(dc,s.c_str(),-1,&r,flags|DT_NOPREFIX);SelectObject(dc,old);}
void rect(HDC dc,int x,int y,int w,int h,COLORREF color){RECT r{px(x),px(y),px(x+w),px(y+h)};auto b=CreateSolidBrush(color);FillRect(dc,&r,b);DeleteObject(b);}
void round(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF stroke,int radius=12){auto b=CreateSolidBrush(fill);auto p=CreatePen(PS_SOLID,px(1),stroke);auto ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,px(x),px(y),px(x+w),px(y+h),px(radius*2),px(radius*2));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
void dot(HDC dc,int x,int y,int radius,COLORREF color){auto b=CreateSolidBrush(color);auto ob=SelectObject(dc,b),op=SelectObject(dc,GetStockObject(NULL_PEN));Ellipse(dc,px(x-radius),px(y-radius),px(x+radius),px(y+radius));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);}
std::wstring moneyText(double n){if(n<0)return L"—";std::wostringstream s;s<<L"$"<<std::fixed<<std::setprecision(1);if(n>=1e9)s<<n/1e9<<L"B";else if(n>=1e6)s<<n/1e6<<L"M";else if(n>=1e3)s<<n/1e3<<L"K";else s<<n;return s.str();}
std::wstring elapsed(const SessionView&v){if(!v.started)return L"00:00";int sec=int((v.ended?v.ended:now())-v.started);wchar_t b[32];swprintf_s(b,L"%02d:%02d:%02d",sec/3600,(sec/60)%60,sec%60);return b;}
void paint(HDC dc){
 rect(dc,0,0,1020,720,bg);rect(dc,0,0,210,720,navy);
 DrawIconEx(dc,px(24),px(30),appIcon,px(56),px(56),0,nullptr,DI_NORMAL);text(dc,L"Zzen",92,38,100,42,28,white,true);
 text(dc,L"YOUR AUTOMATION HUB",25,110,170,22,10,RGB(139,159,181),true);
 round(dc,16,150,178,45,RGB(26,44,62),RGB(26,44,62),9);dot(dc,34,173,4,RGB(58,223,188));text(dc,L"Overview",50,161,130,24,15,white,true);
 text(dc,L"CURRENT PROFILE",25,228,165,20,10,RGB(139,159,181),true);text(dc,L"Army Commander\nTycoon",25,258,165,58,18,white,true);text(dc,L"Roblox · Spanish game UI",25,322,160,38,12,RGB(159,179,199));
 round(dc,20,541,170,85,RGB(23,38,55),RGB(37,54,72),10);round(dc,34,556,35,26,RGB(43,60,79),RGB(43,60,79),5);text(dc,L"F8",34,559,35,20,12,white,true,DT_CENTER);text(dc,L"Start / stop",78,559,102,20,12,white,true);text(dc,L"Shows Roblox on start.",34,594,145,20,11,RGB(163,181,200));
 text(dc,L"v0.1  ·  Windows desktop",25,679,165,20,11,RGB(121,145,168));
 text(dc,L"AUTOMATION",246,31,500,20,11,accent,true);text(dc,L"Your session, under control.",246,58,700,45,28,ink,true);text(dc,L"Press F8 to start. Zzen brings Roblox forward automatically.",246,105,738,22,14,muted);
 round(dc,246,142,738,184,mint,border);
 text(dc,L"Tycoon autopilot",268,158,660,28,20,ink,true);
 text(dc,L"One session. The complete cycle.",268,191,660,24,14,muted);
 const wchar_t* names[3]={L"Stay active",L"Buy upgrades",L"Prestige & repeat"};
 const wchar_t* details[3]={L"Anti-AFK runs automatically.",L"Build with in-game cash.",L"Rebirth, then keep building."};
 for(int i=0;i<3;i++){int x=268+i*232;dot(dc,x+5,238,5,accent);text(dc,names[i],x+18,227,208,25,15,ink,true);text(dc,details[i],x,256,220,36,12,muted);}
 text(dc,L"Prestige resets your buildings and in-game money when ready.",268,300,680,19,11,muted);
 round(dc,246,344,738,157,white,border);
 text(dc,L"SESSION STATUS",268,363,370,20,11,muted,true);
 SessionView v;std::wstring msg;{std::lock_guard<std::mutex>lock(statusMutex);v=sessionView;msg=statusText;}
 std::wstring badge=running?(has(msg,L"Paused")?L"WAITING FOR ROBLOX":L"RUNNING"):(has(msg,L"Diagnostic saved")?L"SAVED":L"STOPPED");
 COLORREF badgeColor=running?accent:muted;if(!running&&has(msg,L"Error"))badgeColor=RGB(181,61,61);
 text(dc,badge,705,363,256,22,11,badgeColor,true,DT_RIGHT);
 if(v.cost>0){double ratio=std::clamp(v.balance/v.cost,0.,1.);round(dc,268,454,694,6,border,border,3);if(ratio>0)round(dc,268,454,std::max(6,int(694*ratio)),6,accent,accent,3);text(dc,(v.saving?L"Saving for promotion   ":L"Last promotion check   ")+moneyText(v.balance)+L" / "+moneyText(v.cost),268,470,570,20,12,muted);text(dc,std::to_wstring(int(ratio*100))+L"%",902,470,60,20,12,accent,true,DT_RIGHT);}
 else text(dc,diagnostic?L"Capture only. No gameplay actions will be sent.":L"Promotion progress appears here after the first check.",268,465,694,22,12,muted);
 int xs[3]={246,497,748};const wchar_t* labels[3]={L"VERIFIED UPGRADES",L"VERIFIED PROMOTIONS",L"SESSION TIME"};std::wstring values[3]={std::to_wstring(v.upgrades),std::to_wstring(v.promotions),elapsed(v)};
 for(int i=0;i<3;i++){round(dc,xs[i],518,236,88,white,border);text(dc,labels[i],xs[i]+18,532,200,20,10,muted,true);text(dc,values[i],xs[i]+18,555,200,36,26,ink,true);}
 text(dc,L"Zzen only acts while Roblox is in the foreground. Switching away pauses inputs.",246,686,738,22,12,muted);
}
void drawButton(const DRAWITEMSTRUCT&d){
 HDC dc=d.hDC;int id=int(d.CtlID),w=int((d.rcItem.right-d.rcItem.left)/scale),h=int((d.rcItem.bottom-d.rcItem.top)/scale);bool disabled=(d.itemState&ODS_DISABLED)!=0,down=(d.itemState&ODS_SELECTED)!=0,focus=(d.itemState&ODS_FOCUS)!=0;
 COLORREF fg=disabled?RGB(142,154,167):ink;
 if(id==20||id==21||id==22){rect(dc,0,0,w,h,navy);text(dc,id==20?L"Quick guide  ↗":id==21?L"Open logs  ↗":L"Export log  ↗",10,8,w-15,h-10,13,RGB(184,205,222));}
 else{COLORREF fill=id==1?(disabled?RGB(206,222,219):down?RGB(0,102,96):accent):white;COLORREF edge=id==1?fill:border;round(dc,0,0,w,h,fill,edge,9);wchar_t label[100];GetWindowTextW(d.hwndItem,label,100);text(dc,label,0,0,w,h,15,id==1?(disabled?RGB(94,125,119):white):fg,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 if(focus){RECT r{px(5),px(5),d.rcItem.right-px(5),d.rcItem.bottom-px(5)};DrawFocusRect(dc,&r);}
}
}
HWND button(const wchar_t*name,int x,int y,int w,int h,int id){auto c=CreateWindowW(L"BUTTON",name,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ui::px(x),ui::px(y),ui::px(w),ui::px(h),mainWindow,(HMENU)INT_PTR(id),GetModuleHandleW(nullptr),nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;}
void refreshUi(){
 {std::lock_guard<std::mutex>lock(statusMutex);SetWindowTextW(statusLabel,statusText.c_str());}
 EnableWindow(startButton,!running&&!starting&&hotkeyReady);EnableWindow(stopButton,running||starting);EnableWindow(analyzeButton,!running&&!starting&&hotkeyReady);
 InvalidateRect(mainWindow,nullptr,FALSE);for(auto h:{startButton,stopButton,analyzeButton})InvalidateRect(h,nullptr,TRUE);
}
void quickGuide(){MessageBoxW(mainWindow,L"1. Open Roblox with the game interface in Spanish.\n2. Start session enables anti-AFK, upgrades and prestige together.\n3. Auto-promotion resets your base and game money when ready.\n4. Press F8 or click Start session. Zzen minimizes and activates Roblox.\n5. Press F8 again to stop; press once more to start again.\n\nTycoon: buys the game's suggested upgrade and checks promotion every 60 seconds.\nAnti-AFK: jumps every 5 minutes while the game toolbar is visible.\n\nZzen pauses when Roblox loses focus. Keep the PC awake and the game visible.\n\nIf something looks wrong: stop, use Run diagnostic, then switch to Roblox. Open logs contains the local diagnostic. Export log creates a text-only feedback file that you can attach to a chat.",L"Zzen · Quick guide",MB_OK|MB_ICONINFORMATION);}
HWND overlayWindow=nullptr;
LRESULT CALLBACK overlayProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_NCHITTEST)return HTTRANSPARENT;
 if(m==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);std::wstring message;SessionView view;{std::lock_guard<std::mutex>guard(statusMutex);message=statusText;view=sessionView;}
  ui::rect(dc,0,0,560,58,ui::navy);ui::dot(dc,16,16,3,RGB(79,218,184));
  ui::text(dc,L"ZZEN  ·  AUTOPILOT",28,7,270,19,10,RGB(186,209,220),true);
  ui::text(dc,L"F8 stop  ·  "+ui::elapsed(view),337,7,207,19,10,RGB(156,178,197),false,DT_RIGHT|DT_SINGLELINE);
  ui::text(dc,message,15,29,530,22,12,RGB(239,246,250),false,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);
  EndPaint(h,&ps);return 0;
 }return DefWindowProcW(h,m,w,l);
}
void refreshOverlay(){
 if(!overlayWindow)return;auto game=focused();
 if(!running||diagnostic||!game){ShowWindow(overlayWindow,SW_HIDE);return;}
 RECT r{};POINT origin{};if(!GetClientRect(game,&r)||!ClientToScreen(game,&origin)){ShowWindow(overlayWindow,SW_HIDE);return;}
 int width=ui::px(560),height=ui::px(58);if(r.right<width+20){ShowWindow(overlayWindow,SW_HIDE);return;}
 SetWindowPos(overlayWindow,HWND_TOPMOST,origin.x+(r.right-width)/2,origin.y+ui::px(10),width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);InvalidateRect(overlayWindow,nullptr,FALSE);
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_COMMAND:{int id=LOWORD(w);if(id==1)startSession();else if(id==2)stop(L"Stopped by you.");else if(id==3)begin(true);else if(id==22)exportLog();else if(id==20)quickGuide();else if(id==21){ShellExecuteW(h,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}return 0;}
 case WM_HOTKEY:if(w==1)toggleSession();return 0;
 case WM_APP:refreshUi();refreshOverlay();return 0;
 case WM_TIMER:{if(w==2){checkStart();return 0;}RECT area{ui::px(246),ui::px(344),ui::px(984),ui::px(607)};InvalidateRect(h,&area,FALSE);refreshOverlay();return 0;}
 case WM_DRAWITEM:ui::drawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(l));return TRUE;
 case WM_CTLCOLORSTATIC:SetBkColor((HDC)w,ui::white);SetTextColor((HDC)w,ui::ink);return (LRESULT)GetStockObject(WHITE_BRUSH);
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC mem=CreateCompatibleDC(dc);auto bmp=CreateCompatibleBitmap(dc,r.right,r.bottom);auto old=SelectObject(mem,bmp);ui::paint(mem);BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);EndPaint(h,&ps);return 0;}
 case WM_CLOSE:stop(L"Application closed.");exiting=true;DestroyWindow(h);return 0;
 case WM_DESTROY:KillTimer(h,1);KillTimer(h,2);if(overlayWindow)DestroyWindow(overlayWindow);overlayWindow=nullptr;UnregisterHotKey(h,1);PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,m,w,l);
}
void initializeUi(HINSTANCE instance){
 ui::scale=GetDpiForSystem()/96.;RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);ui::scale=std::min(ui::scale,(work.bottom-work.top-70)/720.);
 appIcon=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,256,256,LR_DEFAULTCOLOR);

 DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;RECT r{0,0,ui::px(1020),ui::px(720)};AdjustWindowRect(&r,style,FALSE);
 mainWindow=CreateWindowW(L"ZzenAntiAfkNative",L"Zzen",style,80,60,r.right-r.left,r.bottom-r.top,nullptr,nullptr,instance,nullptr);
 SendMessageW(mainWindow,WM_SETICON,ICON_BIG,(LPARAM)appIcon);SendMessageW(mainWindow,WM_SETICON,ICON_SMALL,(LPARAM)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,16,16,LR_DEFAULTCOLOR));
 COLORREF caption=ui::navy,text=ui::white;DwmSetWindowAttribute(mainWindow,35,&caption,sizeof(caption));DwmSetWindowAttribute(mainWindow,36,&text,sizeof(text));
 font=ui::face(16);
 startButton=button(L"Start session · F8",246,630,300,46,1);stopButton=button(L"Stop · F8",562,630,160,46,2);analyzeButton=button(L"Run diagnostic",738,630,246,46,3);
 button(L"Quick guide",24,392,165,38,20);button(L"Open logs",24,434,165,38,21);button(L"Export log",24,476,165,38,22);
 statusLabel=CreateWindowW(L"STATIC",statusText.c_str(),WS_CHILD|WS_VISIBLE|SS_LEFT,ui::px(268),ui::px(397),ui::px(694),ui::px(49),mainWindow,nullptr,instance,nullptr);SendMessageW(statusLabel,WM_SETFONT,(WPARAM)font,TRUE);
 WNDCLASSW overlayClass{};overlayClass.hInstance=instance;overlayClass.lpszClassName=L"ZzenStatusOverlay";overlayClass.lpfnWndProc=overlayProc;RegisterClassW(&overlayClass);
 overlayWindow=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"ZzenStatusOverlay",L"Zzen status",WS_POPUP,0,0,ui::px(560),ui::px(58),nullptr,nullptr,instance,nullptr);
 if(overlayWindow){SetLayeredWindowAttributes(overlayWindow,0,205,LWA_ALPHA);SetWindowRgn(overlayWindow,CreateRoundRectRgn(0,0,ui::px(560),ui::px(58),ui::px(16),ui::px(16)),FALSE);}
 SetTimer(mainWindow,1,500,nullptr);
}
void cleanupUi(){for(auto&entry:ui::fonts)DeleteObject(entry.second);DestroyIcon(appIcon);}
