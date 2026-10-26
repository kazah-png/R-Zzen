#include "vision.hpp"
#include "feedback.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <shellapi.h>
#include <dwmapi.h>
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"dwmapi.lib")
using namespace zzen;
namespace fs=std::filesystem;
HWND mainWindow,statusLabel,startButton,stopButton,analyzeButton;
std::atomic<bool> running=false,exiting=false,diagnostic=false;
std::atomic<unsigned> generation=0;
std::thread worker;std::mutex statusMutex,logMutex;std::wstring statusText=L"Ready when you are. Open Roblox, then start a session.";
fs::path folder;HFONT font;HICON appIcon;
bool hotkeyReady=false;
struct SessionView {int clicks=0,upgrades=0,promotions=0,totalPromotions=-1;bool saving=false;double balance=-1,cost=-1,started=0,ended=0;};
SessionView sessionView;
double now(){return GetTickCount64()/1000.;}
void log(const std::wstring&message){std::lock_guard<std::mutex> guard(logMutex);std::error_code ec;auto logfile=folder/L"sesion-cpp.log";if(fs::exists(logfile,ec)&&fs::file_size(logfile,ec)>5*1024*1024){auto previous=folder/L"sesion-previous.log";fs::remove(previous,ec);fs::rename(logfile,previous,ec);}SYSTEMTIME t;GetLocalTime(&t);char stamp[40];sprintf_s(stamp,"%04u-%02u-%02u %02u:%02u:%02u ",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);std::ofstream out(folder/L"sesion-cpp.log",std::ios::app);out<<stamp<<utf8(message)<<"\n";}
void status(const std::wstring&m,bool record=true){{std::lock_guard<std::mutex> lock(statusMutex);statusText=m;}PostMessageW(mainWindow,WM_APP,0,0);if(record)log(m);}
void stop(const std::wstring&m){running=false;diagnostic=false;generation++;{std::lock_guard<std::mutex>lock(statusMutex);sessionView.ended=now();}status(m);}
HWND focused(){HWND h=GetForegroundWindow();DWORD id=0;GetWindowThreadProcessId(h,&id);HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id);if(!p)return nullptr;wchar_t name[32768];DWORD n=32768;bool ok=QueryFullProcessImageNameW(p,0,name,&n)!=0;CloseHandle(p);return ok&&_wcsicmp(fs::path(name).filename().c_str(),L"RobloxPlayerBeta.exe")==0?h:nullptr;}
Rect bounds(HWND h){RECT r{};POINT p{};if(!GetClientRect(h,&r)||!ClientToScreen(h,&p))throw std::runtime_error("Game window is unavailable");return {p.x,p.y,r.right,r.bottom};}
bool same(Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h;}
bool userInput(){for(int key:{VK_LBUTTON,VK_RBUTTON,VK_SPACE,VK_SHIFT,VK_CONTROL,VK_MENU,VK_LWIN,VK_RWIN})if(GetAsyncKeyState(key)&0x8000)return true;return false;}
Image capture(HWND h,Rect r){
 if(focused()!=h||r.w<900||r.h<600)throw std::runtime_error("Keep Roblox in the foreground, at least 900 x 600 pixels");
 HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=r.w;bi.bmiHeader.biHeight=-r.h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 void* data=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,&data,nullptr,0);
 if(!bitmap){DeleteDC(dc);ReleaseDC(nullptr,screen);throw std::runtime_error("Could not capture Roblox");}
 auto old=SelectObject(dc,bitmap);bool ok=BitBlt(dc,0,0,r.w,r.h,screen,r.x,r.y,SRCCOPY|CAPTUREBLT)!=0;
 Image im{r.w,r.h,std::vector<Pixel>(size_t(r.w)*r.h)};if(ok)memcpy(im.pixels.data(),data,im.pixels.size()*4);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);if(!ok)throw std::runtime_error("Screen capture failed");return im;
}
bool valid(HWND h,Rect b,unsigned gen){return running&&generation==gen&&focused()==h&&same(bounds(h),b);}
void send(INPUT i){if(SendInput(1,&i,sizeof(i))!=1)throw std::runtime_error("Windows rejected the input. Check that both apps run with the same permissions.");}
bool nap(int ms,unsigned gen){for(int elapsed=0;elapsed<ms;elapsed+=25){if(!running||generation!=gen)return false;Sleep(25);}return true;}
bool key(HWND h,Rect b,unsigned gen,UINT vk){
 if(!valid(h,b,gen)||userInput())return false;INPUT i{};i.type=INPUT_KEYBOARD;i.ki.wScan=WORD(MapVirtualKeyW(vk,MAPVK_VK_TO_VSC));i.ki.dwFlags=KEYEVENTF_SCANCODE;send(i);nap(170,gen);i.ki.dwFlags|=KEYEVENTF_KEYUP;SendInput(1,&i,sizeof(i));return true;
}
bool apply(HWND h,Rect b,unsigned gen,const Step&s){
 if(!valid(h,b,gen)||userInput())return false;if(s.action==Action::OpenBuild)return key(h,b,gen,'B');
 if(s.point.x<0||s.point.y<0||s.point.x>=b.w||s.point.y>=b.h)throw std::runtime_error("Target is outside the Roblox window");
 int vx=GetSystemMetrics(SM_XVIRTUALSCREEN),vy=GetSystemMetrics(SM_YVIRTUALSCREEN),vw=GetSystemMetrics(SM_CXVIRTUALSCREEN),vh=GetSystemMetrics(SM_CYVIRTUALSCREEN);int tx=b.x+s.point.x,ty=b.y+s.point.y;
 // Leave the identifying dots unobscured while validating the hovered button.
 if(s.action==Action::OpenMore)tx+=27;
 if(s.action==Action::ConfirmPromotion)tx+=75;
 auto move=[&](int x,int y){INPUT i{};i.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;i.mi.dx=LONG((int64_t(x)-vx)*65536/std::max(1,vw));i.mi.dy=LONG((int64_t(y)-vy)*65536/std::max(1,vh));send(i);};
 move(tx-5,ty);if(!nap(90,gen)||!valid(h,b,gen))return false;move(tx,ty);if(!nap(250,gen)||!valid(h,b,gen)||userInput())return false;
 if(s.action==Action::OpenMore){auto fresh=capture(h,b);auto current=moreDots(fresh);if(!current||abs(current->x-s.point.x)>3||abs(current->y-s.point.y)>3){fs::create_directories(folder/L"diagnostico-cpp");saveImage(fresh,folder/L"diagnostico-cpp"/L"cancelled-target.bmp");log(L"More button changed before click. Input cancelled. Fresh dots="+(current?std::to_wstring(current->x)+L","+std::to_wstring(current->y):L"not found"));if(valid(h,b,gen))move(b.x+b.w/2,b.y+b.h/5);return false;}}
 if(s.action==Action::ConfirmPromotion){auto fresh=capture(h,b);Reader reader;auto current=analyze(fresh,reader.read(fresh));if(!valid(h,b,gen)||!current.confirmation||abs(current.buy.x-s.point.x)>15||abs(current.buy.y-s.point.y)>8){log(L"Promotion confirmation changed. Input cancelled; pending record preserved.");return false;}}
 POINT actual;GetCursorPos(&actual);if(abs(actual.x-tx)>3||abs(actual.y-ty)>3)throw std::runtime_error("The cursor did not reach the button. Turn off mouse lock.");
 INPUT i{};i.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;send(i);nap(110,gen);i.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&i,sizeof(i));
 if(nap(150,gen)&&valid(h,b,gen))move(b.x+b.w/2,b.y+b.h/5);return true;
}
fs::path checkpoint(){return folder/L"promocion-pendiente.txt";}
void persist(const Bot&bot){
 if(bot.promotionPending()){auto tmp=folder/L"promocion-pendiente.tmp";{std::ofstream out(tmp);out<<bot.expectedFrom()<<" "<<bot.confirmationAttempted()<<"\n";out.flush();if(!out)throw std::runtime_error("Could not save the promotion attempt");}if(!MoveFileExW(tmp.c_str(),checkpoint().c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not securely save the promotion record");}
 else if(fs::exists(checkpoint()))fs::remove(checkpoint());
}
void snapshot(const Image&im,const std::vector<Text>&text,const Scene&s,const std::wstring&kind){
 auto dir=folder/L"diagnostico-cpp";fs::create_directories(dir);saveImage(im,dir/(kind+L".bmp"));std::ofstream out(dir/(kind+L".txt"));out<<report(s,text);
}
void loop(bool tycoon,bool promote,bool onlyDiagnostic,unsigned gen){
 try{
  winrt::init_apartment(winrt::apartment_type::multi_threaded);Reader reader;Bot bot;bot.autoPromote=promote;
  if(fs::exists(checkpoint())){std::ifstream in(checkpoint());int count=-1;if(!(in>>count)||count<0)throw std::runtime_error("The pending promotion record could not be read. Check it before continuing.");int confirmed=0;in>>confirmed;bot.recoverPending(count,now(),confirmed!=0);bot.autoPromote=true;status(L"A promotion is pending. Checking its result before making purchases.");}
  int cancelledInputs=0;HWND last=nullptr;double stable=now(),next=onlyDiagnostic?now()+5:now(),anti=now()+300,lastSnap=0,lastDebug=0;std::wstring lastMessage;
  while(running&&generation==gen&&!exiting){
   auto h=focused();if(!h){if(last){bot.focusLost();log(L"Roblox lost focus. Inputs paused.");}last=nullptr;stable=now();if(lastMessage!=L"Paused. Switch to Roblox to continue."){lastMessage=L"Paused. Switch to Roblox to continue.";status(lastMessage);}nap(250,gen);continue;}
   if(h!=last){last=h;stable=now();bot.focusLost();}if(now()-stable<2||now()<next||userInput()){nap(100,gen);continue;}
   Rect b=bounds(h);auto im=capture(h,b);auto text=reader.read(im);auto scene=analyze(im,text);if(!valid(h,b,gen))continue;
   if(now()-lastDebug>=30){lastDebug=now();log(L"DEBUG scene: build="+std::to_wstring(scene.build)+L" promo="+std::to_wstring(scene.promo)+L" hud="+std::to_wstring(scene.hud)+L" more="+std::to_wstring(scene.more)+L" moreOpen="+std::to_wstring(scene.moreOpen)+L" ready="+std::to_wstring(scene.ready)+L" available="+std::to_wstring(scene.available)+L" size="+std::to_wstring(im.w)+L"x"+std::to_wstring(im.h));}
   if(onlyDiagnostic){snapshot(im,text,scene,L"analisis");stop(L"Diagnostic saved. Use Open logs to view the report and screenshot.");break;}
   Step step=tycoon||bot.promotionPending()?bot.next(scene,now()):Step{Action::Wait,{},L"Keep active is on. Jumping every 5 minutes.",1500};
   {std::lock_guard<std::mutex>lock(statusMutex);sessionView.clicks=bot.purchases;sessionView.upgrades=bot.confirmed;sessionView.promotions=bot.promoted;sessionView.saving=bot.saving;if(scene.promo){sessionView.balance=scene.balance.value_or(-1);sessionView.cost=scene.cost.value_or(-1);sessionView.totalPromotions=scene.promotions;}}
   if(now()-lastSnap>=10||step.action!=Action::Wait){snapshot(im,text,scene,step.action==Action::Promote?L"antes-promocion":step.action==Action::ConfirmPromotion?L"antes-confirmacion":L"estado");lastSnap=now();}
   if(step.message!=lastMessage||step.action!=Action::Wait){lastMessage=step.message;status(step.message);}
   if(step.action==Action::Stop){stop(step.message);break;}
   if(step.action!=Action::Wait){
    if(step.action==Action::Promote||step.action==Action::ConfirmPromotion)persist(bot);bool sent=apply(h,b,gen,step);bot.acknowledge(step,sent);if(step.action==Action::ConfirmPromotion&&!sent)persist(bot);if(sent)cancelledInputs=0;else if(++cancelledInputs>=5){stop(L"Inputs cancelled five times. Check the toolbar and mouse, then restart.");break;}log(L"Input "+std::to_wstring(int(step.action))+(sent?L" sent":L" cancelled")+L" target="+std::to_wstring(step.point.x)+L","+std::to_wstring(step.point.y));
    if(step.action==Action::Promote)log(L"PROMOCION_INTENTO contador_previo="+std::to_wstring(bot.expectedFrom()));
   }
   if(!bot.promotionPending()&&fs::exists(checkpoint())){persist(bot);snapshot(im,text,scene,L"promocion-verificada");log(L"PROMOCION_VERIFICADA contador="+std::to_wstring(scene.promotions));}
   if(now()>=anti&&scene.hud&&!scene.more&&!scene.moreOpen&&!bot.promotionPending()&&step.action==Action::Wait){if(key(h,b,gen,VK_SPACE)){anti=now()+300;log(L"Keep active: jump sent.");}}
   next=now()+step.delay/1000.;
  }
 }catch(const winrt::hresult_error&e){log(L"OCR/Windows detail: "+std::wstring(e.message()));stop(L"Windows text recognition failed. Use Open logs for details.");}catch(const std::exception&e){stop(L"Error: "+std::wstring(winrt::to_hstring(e.what())));}
 running=false;PostMessageW(mainWindow,WM_APP,0,0);
}
void begin(bool diag){if(running||!hotkeyReady)return;if(worker.joinable())worker.join();{std::lock_guard<std::mutex>lock(statusMutex);sessionView={};sessionView.started=now();}running=true;diagnostic=diag;unsigned gen=++generation;log(L"SESSION Zzen 0.1 | native x64 | unified autopilot | diagnostic="+std::to_wstring(diag));status(diag?L"Diagnostic in 5 seconds. Switch to Roblox.":L"Session started. Switch to Roblox. Press F8 to stop.");worker=std::thread(loop,true,true,diag,gen);}
void exportLog(){
 try{std::wstring message;SessionView view;{std::lock_guard<std::mutex>guard(statusMutex);message=statusText;view=sessionView;}
  auto dir=folder/L"feedback";fs::create_directories(dir);SYSTEMTIME t;GetLocalTime(&t);wchar_t name[100];swprintf_s(name,L"Zzen-feedback-%04u%02u%02u-%02u%02u%02u-%03u.txt",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);auto path=dir/name;
  std::ostringstream summary;summary<<"Status: "<<utf8(message)<<"\nRunning: "<<running<<"\nVerified upgrades: "<<view.upgrades<<"\nVerified promotions: "<<view.promotions<<"\nPromotion count: "<<view.totalPromotions<<"\nSaving: "<<view.saving<<"\nLast balance: "<<view.balance<<"\nLast cost: "<<view.cost<<"\nPending promotion record: "<<fs::exists(checkpoint())<<"\n";
  {std::lock_guard<std::mutex>guard(logMutex);writeFeedback(path,summary.str(),logTail(folder/L"sesion-cpp.log"));}
  log(L"Feedback text exported.");ShellExecuteW(mainWindow,L"open",dir.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
  MessageBoxW(mainWindow,(L"Your feedback log is ready.\n\n"+std::wstring(name)+L"\n\nAttach this text file to the chat. It contains events and errors only, with no screenshots. Nothing has been uploaded.").c_str(),L"Zzen - Export log",MB_OK|MB_ICONINFORMATION);
 }catch(const std::exception&e){MessageBoxW(mainWindow,winrt::to_hstring(e.what()).c_str(),L"Zzen - Export failed",MB_OK|MB_ICONERROR);}
}
#include "ui.hpp"
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
 SetProcessDPIAware();wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);folder=fs::path(path).parent_path();
 HANDLE singleton=CreateMutexW(nullptr,TRUE,L"Local\\zzen-antiafk-native-v2");if(GetLastError()==ERROR_ALREADY_EXISTS){MessageBoxW(nullptr,L"Zzen is already open. Check your taskbar.",L"Zzen",MB_OK);CloseHandle(singleton);return 0;}
 WNDCLASSW c{};c.hInstance=instance;c.lpszClassName=L"ZzenAntiAfkNative";c.lpfnWndProc=proc;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);
 initializeUi(instance);
 hotkeyReady=RegisterHotKey(mainWindow,1,MOD_NOREPEAT,VK_F8)!=0;
 if(!hotkeyReady)status(L"F8 is in use. Close the other automation app and reopen Zzen.");
 refreshUi();
 ShowWindow(mainWindow,show);UpdateWindow(mainWindow);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(mainWindow,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 running=false;generation++;if(worker.joinable())worker.join();cleanupUi();ReleaseMutex(singleton);CloseHandle(singleton);return 0;
}
