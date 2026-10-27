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
std::atomic<bool> starting=false;
std::atomic<bool> workerDone=true;
HWND startTarget=nullptr;double startDeadline=0;
std::atomic<unsigned> generation=0;
std::thread worker;std::mutex statusMutex,logMutex;std::wstring statusText=L"Ready when you are. Open Roblox, then start a session.";
fs::path folder;HFONT font;HICON appIcon;
bool hotkeyReady=false;
struct SessionView {int clicks=0,upgrades=0,promotions=0,totalPromotions=-1,recoveries=0,ocrErrors=0;bool saving=false,attention=false;double balance=-1,cost=-1,started=0,ended=0,lastKeepAlive=0;};
SessionView sessionView;
double now(){return GetTickCount64()/1000.;}
void log(const std::wstring&message){std::lock_guard<std::mutex> guard(logMutex);std::error_code ec;auto logfile=folder/L"sesion-cpp.log";if(fs::exists(logfile,ec)&&fs::file_size(logfile,ec)>5*1024*1024){auto previous=folder/L"sesion-previous.log";fs::remove(previous,ec);fs::rename(logfile,previous,ec);}SYSTEMTIME t;GetLocalTime(&t);char stamp[40];sprintf_s(stamp,"%04u-%02u-%02u %02u:%02u:%02u ",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);std::ofstream out(folder/L"sesion-cpp.log",std::ios::app);out<<stamp<<utf8(message)<<"\n";}
void status(const std::wstring&m,bool record=true){{std::lock_guard<std::mutex> lock(statusMutex);statusText=m;}PostMessageW(mainWindow,WM_APP,0,0);if(record)log(m);}
void stop(const std::wstring&m){starting=false;running=false;diagnostic=false;generation++;{std::lock_guard<std::mutex>lock(statusMutex);sessionView.ended=now();}status(m);}
bool isRoblox(HWND h){
 if(!h||!IsWindow(h))return false;DWORD id=0;GetWindowThreadProcessId(h,&id);
 HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id);if(!p)return false;
 wchar_t name[32768];DWORD n=32768;bool ok=QueryFullProcessImageNameW(p,0,name,&n)!=0;CloseHandle(p);
 return ok&&_wcsicmp(fs::path(name).filename().c_str(),L"RobloxPlayerBeta.exe")==0;
}
HWND focused(){auto h=GetForegroundWindow();return isRoblox(h)?h:nullptr;}
BOOL CALLBACK collectRobloxWindows(HWND h,LPARAM context){
 if(IsWindowVisible(h)&&!GetWindow(h,GW_OWNER)&&!(GetWindowLongPtrW(h,GWL_EXSTYLE)&WS_EX_TOOLWINDOW)&&isRoblox(h))
  reinterpret_cast<std::vector<HWND>*>(context)->push_back(h);
 return TRUE;
}
// Called only in response to an explicit Start/F8 action, never from the worker.
HWND findRoblox(){
 std::vector<HWND> games;EnumWindows(collectRobloxWindows,reinterpret_cast<LPARAM>(&games));
 HWND target=focused();
 if(!target){
  if(games.empty()){status(L"Open Roblox before starting. No game window was found.");return nullptr;}
  if(games.size()!=1){status(L"Several Roblox windows are open. Select the game, then press F8.");return nullptr;}
  target=games.front();
 }
 return target;
}
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
bool apply(HWND h,Rect b,unsigned gen,const Step&s,const Scene&before){
 if(!valid(h,b,gen)||userInput())return false;if(s.action==Action::OpenBuild)return key(h,b,gen,'B');
 if(s.point.x<0||s.point.y<0||s.point.x>=b.w||s.point.y>=b.h)throw std::runtime_error("Target is outside the Roblox window");
 int vx=GetSystemMetrics(SM_XVIRTUALSCREEN),vy=GetSystemMetrics(SM_YVIRTUALSCREEN),vw=GetSystemMetrics(SM_CXVIRTUALSCREEN),vh=GetSystemMetrics(SM_CYVIRTUALSCREEN);int tx=b.x+s.point.x,ty=b.y+s.point.y;
 // Leave the identifying dots unobscured while validating the hovered button.
 if(s.action==Action::OpenMore)tx+=27;
 // Keep the pointer away from the words and price during the fresh OCR check.
 if(s.action==Action::ClickBuy)tx-=int(120*before.layout.s);
 if(s.action==Action::Promote)tx-=int(110*before.layout.s);
 if(s.action==Action::ConfirmPromotion)tx+=75;
 auto move=[&](int x,int y){INPUT i{};i.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;i.mi.dx=LONG((int64_t(x)-vx)*65536/std::max(1,vw));i.mi.dy=LONG((int64_t(y)-vy)*65536/std::max(1,vh));send(i);};
 move(tx-5,ty);if(!nap(90,gen)||!valid(h,b,gen))return false;move(tx,ty);if(!nap(250,gen)||!valid(h,b,gen)||userInput())return false;
 if(s.action==Action::OpenMore){auto fresh=capture(h,b);auto current=moreDots(fresh);if(!current||abs(current->x-s.point.x)>3||abs(current->y-s.point.y)>3){fs::create_directories(folder/L"diagnostico-cpp");saveImage(fresh,folder/L"diagnostico-cpp"/L"cancelled-target.bmp");log(L"More button changed before click. Input cancelled. Fresh dots="+(current?std::to_wstring(current->x)+L","+std::to_wstring(current->y):L"not found"));if(valid(h,b,gen))move(b.x+b.w/2,b.y+b.h/5);return false;}}
 if(s.action!=Action::OpenMore){auto fresh=capture(h,b);Reader reader([&]{return !running||generation!=gen||focused()!=h;});auto lines=reader.read(fresh);auto current=analyze(fresh,lines);if(!valid(h,b,gen)||!sameTarget(before,current,s)){log(L"Target changed before click. Input cancelled; re-reading the current menu.");fs::create_directories(folder/L"diagnostico-cpp");saveImage(fresh,folder/L"diagnostico-cpp"/L"cancelled-action.bmp");{std::ofstream out(folder/L"diagnostico-cpp"/L"cancelled-action.txt");out<<report(current,lines);}if(valid(h,b,gen))move(b.x+b.w/2,b.y+b.h/5);return false;}}
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
 SetThreadExecutionState(ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED);
 try{
  winrt::init_apartment(winrt::apartment_type::multi_threaded);Reader reader([&]{return !running||generation!=gen||!focused();});Bot bot;bot.autoPromote=promote;
  if(fs::exists(checkpoint())){std::ifstream in(checkpoint());int count=-1;if(!(in>>count)||count<0)throw std::runtime_error("The pending promotion record could not be read. Check it before continuing.");int confirmed=0;in>>confirmed;bot.recoverPending(count,now(),confirmed!=0);bot.autoPromote=true;status(L"A promotion is pending. Checking its result before making purchases.");}
  int cancelledInputs=0;HWND last=nullptr;double stable=now(),next=onlyDiagnostic?now()+5:now(),lastSnap=0,lastDebug=0,lastPurchase=now();std::wstring lastMessage,attentionReason;KeepAlive keepAlive(now());int lastPurchases=0,ocrErrors=0;
  while(running&&generation==gen&&!exiting){
   auto h=focused();if(!h){if(last){bot.focusLost();log(L"Roblox lost focus. Inputs paused.");}last=nullptr;stable=now();if(lastMessage!=L"Paused. Switch to Roblox to continue."){lastMessage=L"Paused. Switch to Roblox to continue.";status(lastMessage);}nap(250,gen);continue;}
   if(h!=last){last=h;stable=now();bot.focusLost();}if(now()-stable<2||now()<next||userInput()){nap(100,gen);continue;}
   Rect b=bounds(h);if(b.w<900||b.h<600){status(L"Paused: Roblox must be at least 900 x 600 pixels.",false);nap(1000,gen);continue;}
   Image im;std::vector<Text> text;Scene scene;
   try{im=capture(h,b);text=reader.read(im);scene=analyze(im,text);}
   catch(const std::exception&e){if(!running||generation!=gen)break;if(!focused())continue;++ocrErrors;{std::lock_guard<std::mutex>lock(statusMutex);sessionView.ocrErrors=ocrErrors;sessionView.attention=true;}status(L"Reading failed. Retrying automatically; no input sent.");log(L"READ_ERROR "+std::wstring(winrt::to_hstring(e.what())));nap(1500,gen);continue;}
   catch(const winrt::hresult_error&e){if(!running||generation!=gen)break;if(!focused())continue;++ocrErrors;{std::lock_guard<std::mutex>lock(statusMutex);sessionView.ocrErrors=ocrErrors;sessionView.attention=true;}status(L"Windows OCR failed. Retrying automatically.");log(L"OCR_ERROR "+std::wstring(e.message()));nap(1500,gen);continue;}
   if(!valid(h,b,gen))continue;
   if(now()-lastDebug>=30){lastDebug=now();log(L"DEBUG scene: build="+std::to_wstring(scene.build)+L" promo="+std::to_wstring(scene.promo)+L" hud="+std::to_wstring(scene.hud)+L" more="+std::to_wstring(scene.more)+L" moreOpen="+std::to_wstring(scene.moreOpen)+L" ready="+std::to_wstring(scene.ready)+L" available="+std::to_wstring(scene.available)+L" size="+std::to_wstring(im.w)+L"x"+std::to_wstring(im.h));}
   if(onlyDiagnostic){snapshot(im,text,scene,L"analisis");stop(L"Diagnostic saved. Use Open logs to view the report and screenshot.");break;}
   // Keep-active has its own clock and priority over menu work, including recovery.
   if(keepAlive.ready(scene,now())&&key(h,b,gen,VK_SPACE)){keepAlive.sent(now());{std::lock_guard<std::mutex>lock(statusMutex);sessionView.lastKeepAlive=now();}log(L"Keep active: jump sent.");next=now()+.5;continue;}
   Step step=attentionReason.empty()?(tycoon||bot.promotionPending()?bot.next(scene,now()):Step{Action::Wait,{},L"Keep active is on.",1500}):Step{Action::Wait,{},attentionReason,1500};
   if(bot.purchases!=lastPurchases){lastPurchases=bot.purchases;lastPurchase=now();}
   if(!bot.saving&&!bot.promotionPending()&&scene.build&&scene.buyEnabled&&!scene.available&&now()-lastPurchase>120)step.message=L"Needs attention: no purchase progress. Retrying recognition automatically.";
   if(step.action==Action::Stop){attentionReason=L"Needs attention: "+step.message;step={Action::Wait,{},attentionReason,1500};snapshot(im,text,scene,L"attention");log(L"AUTOPILOT_SUSPENDED: "+attentionReason);}
   {std::lock_guard<std::mutex>lock(statusMutex);sessionView.clicks=bot.purchases;sessionView.upgrades=bot.confirmed;sessionView.promotions=bot.promoted;sessionView.saving=bot.saving;sessionView.recoveries=bot.recoveries;sessionView.ocrErrors=ocrErrors;sessionView.attention=!attentionReason.empty()||has(step.message,L"Needs attention");if(scene.promo){sessionView.balance=scene.balance.value_or(-1);sessionView.cost=scene.cost.value_or(-1);sessionView.totalPromotions=scene.promotions;}}
   if(now()-lastSnap>=10||step.action!=Action::Wait){snapshot(im,text,scene,step.action==Action::Promote?L"antes-promocion":step.action==Action::ConfirmPromotion?L"antes-confirmacion":L"estado");lastSnap=now();}
   if(step.message!=lastMessage||step.action!=Action::Wait){lastMessage=step.message;status(step.message);}
   if(step.action!=Action::Wait){
    if(step.action==Action::Promote||step.action==Action::ConfirmPromotion)persist(bot);bool sent=false;
    try{sent=apply(h,b,gen,step,scene);}catch(const std::exception&e){log(L"INPUT_ERROR "+std::wstring(winrt::to_hstring(e.what())));}catch(const winrt::hresult_error&e){log(L"INPUT_OCR_ERROR "+std::wstring(e.message()));}
    bot.acknowledge(step,sent);if(step.action==Action::ConfirmPromotion&&!sent)persist(bot);if(sent)cancelledInputs=0;else if(++cancelledInputs>=5){bot.retryNavigation(now());cancelledInputs=0;log(L"Repeated cancelled targets. Starting menu recovery.");}log(L"Input "+std::to_wstring(int(step.action))+(sent?L" sent":L" cancelled")+L" target="+std::to_wstring(step.point.x)+L","+std::to_wstring(step.point.y));
    if(step.action==Action::Promote)log(L"PROMOCION_INTENTO contador_previo="+std::to_wstring(bot.expectedFrom()));
   }
   if(!bot.promotionPending()&&fs::exists(checkpoint())){persist(bot);snapshot(im,text,scene,L"promocion-verificada");log(L"PROMOCION_VERIFICADA contador="+std::to_wstring(scene.promotions));}
   next=now()+step.delay/1000.;
  }
 }catch(const winrt::hresult_error&e){log(L"OCR/Windows detail: "+std::wstring(e.message()));stop(L"Windows text recognition failed. Use Open logs for details.");}catch(const std::exception&e){stop(L"Error: "+std::wstring(winrt::to_hstring(e.what())));}
 SetThreadExecutionState(ES_CONTINUOUS);running=false;workerDone=true;PostMessageW(mainWindow,WM_APP,0,0);
}
void begin(bool diag){if(running||!hotkeyReady)return;if(!workerDone){status(L"Finishing the previous session. Try again in a moment.");return;}if(worker.joinable())worker.join();{std::lock_guard<std::mutex>lock(statusMutex);sessionView={};sessionView.started=now();}workerDone=false;running=true;diagnostic=diag;unsigned gen=++generation;log(L"SESSION Zzen 0.2 | native x64 | unified autopilot | diagnostic="+std::to_wstring(diag));status(diag?L"Diagnostic in 5 seconds. Switch to Roblox.":L"Session started. Press F8 to stop.");worker=std::thread(loop,true,true,diag,gen);}
void checkStart(){
 if(!starting){KillTimer(mainWindow,2);return;}
 // Activation across UI threads is asynchronous. Observe it without stealing focus again.
 if(focused()==startTarget&&workerDone){
  starting=false;KillTimer(mainWindow,2);
  log(L"Roblox activated; Zzen minimized by explicit Start/F8 action.");begin(false);return;
 }
 if(now()>=startDeadline||!isRoblox(startTarget)){
  starting=false;KillTimer(mainWindow,2);
  ShowWindow(mainWindow,SW_SHOWNOACTIVATE);
  status(L"Could not activate Roblox. Select the game and press F8 again.");
 }
}
void startSession(){
 if(running||starting||!hotkeyReady)return;
 auto target=findRoblox();if(!target)return;
 startTarget=target;startDeadline=now()+1.5;starting=true;
 if(!SetTimer(mainWindow,2,100,nullptr)){starting=false;status(L"Could not start the activation timer. Please reopen Zzen.");return;}
 status(L"Bringing Roblox forward...");
 ShowWindow(mainWindow,SW_MINIMIZE);
 if(IsIconic(target))ShowWindow(target,SW_RESTORE);
 SetForegroundWindow(target);
}
void toggleSession(){
 if(running||starting)stop(L"Stopped with F8. Press F8 to start again.");
 else startSession();
}
void exportLog(){
 try{std::wstring message;SessionView view;{std::lock_guard<std::mutex>guard(statusMutex);message=statusText;view=sessionView;}
  auto dir=folder/L"feedback";fs::create_directories(dir);SYSTEMTIME t;GetLocalTime(&t);wchar_t name[100];swprintf_s(name,L"Zzen-feedback-%04u%02u%02u-%02u%02u%02u-%03u.txt",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);auto path=dir/name;
  std::ostringstream summary;summary<<"Status: "<<utf8(message)<<"\nRunning: "<<running<<"\nVerified upgrades: "<<view.upgrades<<"\nVerified promotions: "<<view.promotions<<"\nPromotion count: "<<view.totalPromotions<<"\nSaving: "<<view.saving<<"\nLast balance: "<<view.balance<<"\nLast cost: "<<view.cost<<"\nPending promotion record: "<<fs::exists(checkpoint())<<"\n";
  summary<<"Menu recoveries: "<<view.recoveries<<"\nOCR errors: "<<view.ocrErrors<<"\nNeeds attention: "<<view.attention<<"\nPurchase inputs: "<<view.clicks<<"\nLast keep-active age (seconds, -1 if none): "<<(view.lastKeepAlive?now()-view.lastKeepAlive:-1)<<"\n";
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
