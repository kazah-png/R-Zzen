#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <optional>
#include <regex>
#include <string>
#include <vector>
namespace zzen {
struct Point {int x=0,y=0;};
struct Rect {int x=0,y=0,w=0,h=0;bool contains(Point p)const{return p.x>=x&&p.y>=y&&p.x<x+w&&p.y<y+h;}Point center()const{return{x+w/2,y+h/2};}};
inline std::wstring norm(std::wstring s){for(auto& c:s){c=towlower(c);auto p=std::wstring(L"áéíóúüñÁÉÍÓÚÜÑ").find(c);if(p!=std::wstring::npos)c=L"aeiouunaeiouun"[p];}return s;}
inline bool has(const std::wstring&s,const std::wstring&part){return norm(s).find(norm(part))!=std::wstring::npos;}
struct Text {std::wstring text;Rect box;};
struct Pixel {uint8_t b,g,r,a;};
struct Image {int w=0,h=0;std::vector<Pixel> pixels;Pixel at(int x,int y)const{return (x>=0&&y>=0&&x<w&&y<h)?pixels[y*w+x]:Pixel{};}};
inline bool yellow(Pixel c){return c.r>185&&c.g>165&&c.b<150&&c.r>c.b*1.3;}
inline double yellowRatio(const Image&im,Rect r){int n=0,k=0;for(int y=std::max(0,r.y);y<std::min(im.h,r.y+r.h);y+=3)for(int x=std::max(0,r.x);x<std::min(im.w,r.x+r.w);x+=3){n++;k+=yellow(im.at(x,y));}return n?double(k)/n:0;}
inline std::wstring within(const std::vector<Text>&t,Rect r){std::wstring s;for(auto&l:t)if(r.contains(l.box.center()))s+=l.text+L" ";return s;}
inline bool hasPrice(const std::wstring&s){return std::regex_search(s,std::wregex(LR"([$sS]\s*[0-9Il|Oo][0-9Il|Oo.,]*\s*[kKmMbBtT]?\b)"));}
inline std::optional<double> money(const std::wstring&s){
 std::wsmatch m;if(!std::regex_search(s,m,std::wregex(LR"([$sS]\s*([0-9Il|Oo][0-9Il|Oo.,]*)\s*([kKmMbBtT]?))")))return{};
 auto n=m[1].str();for(auto&c:n){if(c==L'O'||c==L'o')c=L'0';if(c==L'I'||c==L'l'||c==L'|')c=L'1';}
 auto unit=norm(m[2].str());if(unit.empty())n.erase(std::remove(n.begin(),n.end(),L','),n.end());else std::replace(n.begin(),n.end(),L',',L'.');
 size_t end=size_t(m.position()+m.length());if(end<s.size()&&(iswalnum(s[end])||s[end]==L'.'||s[end]==L','))return{};
 try{size_t used=0;double v=std::stod(n,&used);if(used!=n.size()||v<0)return{};if(unit==L"k")v*=1e3;else if(unit==L"m")v*=1e6;else if(unit==L"b")v*=1e9;else if(unit==L"t")v*=1e12;return v;}catch(...){return{};}
}
struct Layout {double s=1;int x=0,y=0;Rect area(int a,int b,int w,int h)const{return{x+int(a*s),y+int(b*s),int(w*s),int(h*s)};}Point at(int a,int b)const{return{x+int(a*s),y+int(b*s)};}};
inline std::optional<Layout> buildLayout(const std::vector<Text>&t,int w,int h){
 for(auto&l:t)if(norm(l.text)==L"construccion"){
  double s=l.box.w/171.;Layout a{s,l.box.x-int(42*s),l.box.y-int(s)};
  if(s<.6||s>2.5||a.x< -30*s||a.y< -12*s||a.at(1325,775).x>w||a.at(1325,775).y>h)continue;
  unsigned mask=0;for(auto&v:t)if(a.area(15,90,690,55).contains(v.box.center())){auto k=L" "+norm(v.text)+L" ";unsigned bit=1;for(auto word:{L"todos",L"economia",L"naval",L"soporte",L"defensa",L"ejercito",L"vehiculos",L"aire"}){if(k.find(L" "+std::wstring(word)+L" ")!=std::wstring::npos)mask|=bit;bit<<=1;}}
  int tabs=0;for(;mask;mask>>=1)tabs+=mask&1;if(tabs>=3)return a;
 }return{};
}
inline std::optional<Layout> promoLayout(const std::vector<Text>&t,int w,int h){
 for(auto&l:t)if(norm(l.text)==L"promocion"){
  double s=l.box.w/120.;Layout a{s,l.box.x-int(42*s),l.box.y-int(12*s)};
  if(s<.6||s>2.5||a.x<0||a.y<0||a.at(748,580).x>w||a.at(748,580).y>h)continue;
  auto details=within(t,a.area(25,30,710,210));
  if(has(details,L"renacimiento")&&(has(details,L"promociones")||has(details,L"promotions so far")))return a;
 }return{};
}
inline bool notice(const Image&im,Rect region){
 for(int r=7;r<=25;r+=2)for(int y=std::max(r,region.y+r);y<std::min(im.h-r,region.y+region.h-r);y+=2)for(int x=std::max(r,region.x+r);x<std::min(im.w-r,region.x+region.w-r);x+=2){
  int ye=0,re=0;for(int i=0;i<12;i++){double a=i*3.141592653589793/6;auto c=im.at(x+int(cos(a)*r*.70),y+int(sin(a)*r*.70));ye+=yellow(c);re+=(c.r>190&&c.g>65&&c.g<190&&c.b>65&&c.b<190&&c.r>c.g*1.3);}
  if(ye>=11){auto c=im.at(x,y);if(c.r<105&&c.g<105&&c.b<105)return true;}
  if(re>=10){int white=0;for(int j=-int(r*.45);j<r*.5;j++){auto c=im.at(x,y+j);white+=(c.r>210&&c.g>205&&c.b>205);}if(white>=r*.5)return true;}
 }return false;
}
struct Scene {
 bool build=false,promo=false,hud=false,partial=false,available=false,buyEnabled=false,notice=false,blocked=false,ready=false,more=false,moreOpen=false,confirmation=false;
 Layout layout;Point buy{},close{},morePoint{},promoPoint{};
 std::wstring name,detail;int level=-1,promotions=-1;std::optional<double> balance,cost;
};
inline std::optional<Point> moreDots(const Image&im){
 // Three equal white disks on the dark toolbar button; no fixed button index.
 int x0=int(im.w*.2),x1=int(im.w*.4),y0=int(im.h*.88),y1=int(im.h*.965),rw=x1-x0,rh=y1-y0;
 if(rw<1||rh<1)return{};std::vector<uint8_t>seen(size_t(rw)*rh);std::vector<Rect> disks;
 auto white=[&](int x,int y){auto p=im.at(x,y);return p.r>170&&p.g>205&&p.b>235;};
 for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){int id=(y-y0)*rw+x-x0;if(seen[id]||!white(x,y))continue;std::vector<Point>q{{x,y}};seen[id]=1;int left=x,right=x,top=y,bottom=y;
  for(size_t i=0;i<q.size();i++){auto p=q[i];left=std::min(left,p.x);right=std::max(right,p.x);top=std::min(top,p.y);bottom=std::max(bottom,p.y);for(auto d:{Point{1,0},Point{-1,0},Point{0,1},Point{0,-1}}){int nx=p.x+d.x,ny=p.y+d.y;if(nx<x0||nx>=x1||ny<y0||ny>=y1)continue;int j=(ny-y0)*rw+nx-x0;if(!seen[j]&&white(nx,ny)){seen[j]=1;q.push_back({nx,ny});}}}
  int w=right-left+1,h=bottom-top+1;if(w>=5&&w<=18&&h>=5&&h<=18&&abs(w-h)<=2&&q.size()>size_t(w*h*.60))disks.push_back({left,top,w,h});
 }
 std::sort(disks.begin(),disks.end(),[](auto&a,auto&b){return a.x<b.x;});
 for(auto&a:disks)for(auto&b:disks)for(auto&c:disks){int d=b.center().x-a.center().x,e=c.center().x-b.center().x;if(d<a.w*1.5||d>a.w*3.2||abs(d-e)>2||abs(a.center().y-b.center().y)>2||abs(b.center().y-c.center().y)>2||abs(a.w-b.w)>2||abs(b.w-c.w)>2)continue;auto bg=im.at(b.center().x,b.y-b.h);if(bg.r>20&&bg.r<130&&bg.g>30&&bg.g<145&&bg.b>40&&bg.b<160&&abs(int(bg.r)-bg.g)<40)return b.center();}
 return{};
}
inline Scene analyze(const Image&im,const std::vector<Text>&t){
 Scene z;std::wstring all;for(auto&l:t)all+=l.text+L" ";
 z.blocked=has(all,L"robux")||has(all,L"desconectado")||has(all,L"reconnect")||has(all,L"reconectar");
 z.balance=money(within(t,{100,0,int(im.w*.25),int(im.h*.065)}));
 // Recognise the rebirth confirmation independently of the obscured panel behind it.
 Rect center{int(im.w*.30),int(im.h*.34),int(im.w*.40),int(im.h*.36)};
 auto dialog=norm(within(t,center));
 if(has(dialog,L"promote")&&has(dialog,L"tu base y dinero se reinician")&&has(dialog,L"aumentan permanentemente")){
  for(auto&yes:t)if(center.contains(yes.box.center())&&norm(yes.text)==L"promocionar")
   for(auto&no:t)if(center.contains(no.box.center())&&norm(no.text)==L"cancelar"){
    auto a=no.box.center(),b=yes.box.center();
    if(b.x>a.x+im.w*.10&&abs(a.y-b.y)<12&&b.y>im.h*.55&&yellowRatio(im,{b.x-65,b.y-24,130,48})>.6){z.confirmation=true;z.buy=b;return z;}
   }
 }
 if(auto p=promoLayout(t,im.w,im.h)){
  z.promo=true;z.layout=*p;z.close=p->at(716,39);auto r=p->area(216,492,328,76);z.buy=r.center();
  auto desc=norm(within(t,p->area(25,130,710,70)));std::wsmatch m;
  if(std::regex_search(desc,m,std::wregex(LR"((?:promociones\s+hasta\s+ahora|promotions\s+so\s+far)\s*:\s*(\d+))")))z.promotions=std::stoi(m[1]);
  auto button=within(t,r);z.cost=money(button);
  // The game replaces the citadel requirement with this message once affordable.
  z.ready=(has(desc,L"construida")||has(desc,L"todo esta listo: presiona promocionar"))&&!has(desc,L"necesita");
  auto funds=desc.find(L"tienes");if(funds!=std::wstring::npos)z.balance=money(desc.substr(funds));
  z.available=(has(button,L"promocionar")||has(button,L"promojonar"))&&z.cost&&z.promotions>=0&&z.ready&&yellowRatio(im,r)>.5&&(!z.balance||*z.balance>=*z.cost);
  z.detail=desc;return z;
 }
 if(auto b=buildLayout(t,im.w,im.h)){
  z.build=true;z.layout=*b;z.close=b->at(1301,33);auto r=b->area(925,685,380,80);z.buy=r.center();auto txt=within(t,r);
  z.buyEnabled=yellowRatio(im,b->area(936,692,360,62))>.55&&(has(txt,L"mejor")||has(txt,L"construir")||has(txt,L"comprar")||has(txt,L"build")||has(txt,L"upgrade"))&&!has(txt,L"necesita")&&!has(txt,L"requiere");
  z.available=z.buyEnabled&&(hasPrice(txt)||std::regex_search(norm(txt),std::wregex(LR"(\b(?:build|upgrade)\s+[0-9][0-9.,]*\b)")));
  for(auto&line:t)if(b->area(920,330,390,47).contains(line.box.center()))z.name=norm(line.text);
  z.cost=money(txt);z.detail=txt;
  std::wsmatch m;auto lev=norm(within(t,b->area(920,378,390,32)));if(std::regex_search(lev,m,std::wregex(LR"((?:nivel|level)\s*([0-9il|]+)\s*/\s*\d+)"))){auto digits=m[1].str();for(auto&digit:digits)if(digit<L'0'||digit>L'9')digit=L'1';z.level=std::stoi(digits);}else if(has(lev,L"no construido")||has(lev,L"not built"))z.level=0;return z;
 }
 for(auto&l:t)if((has(l.text,L"construccion")||norm(l.text)==L"promocion")&&l.box.y<im.h*.65)z.partial=true;
 const Text*anchor=nullptr;int companionMask=0;
 for(auto&l:t){auto k=norm(l.text);if(l.box.y>im.h*.65&&l.box.x<im.w*.4){if((k==L"construir"||k==L"consiruir")&&l.box.x<im.w*.2)anchor=&l;if(k==L"ejercito")companionMask|=1;if(k==L"comando")companionMask|=2;if(k==L"tienda")companionMask|=4;}
 }
 if(anchor&&companionMask){if(auto p=moreDots(im)){z.morePoint=*p;int matches=0;for(auto word:{L"mapa",L"equipamiento",L"rivales",L"regiment"}){bool found=false;Rect row{std::max(0,p->x-250),p->y-90,500,48};for(auto&label:t)if(row.contains(label.box.center())&&norm(label.text)==word)found=true;if(found)matches++;}z.moreOpen=matches>=2;}}
 // Hover changes label height. Anchor the icon to the current toolbar, not
 // font geometry; still require the verified popup and its promotion column.
 if(z.moreOpen){Rect labelArea{z.morePoint.x+45,z.morePoint.y-90,100,42};for(auto&label:t)if(labelArea.contains(label.box.center())){auto k=norm(label.text);k.erase(std::remove_if(k.begin(),k.end(),[](wchar_t c){return !iswalpha(c);}),k.end());if(k.size()>=7&&k.size()<=14&&(k.rfind(L"promo",0)==0||k==L"preconar"||k==L"precknar"||k==L"precionar"||k==L"procionar")&&k.substr(k.size()-3)==L"nar"){z.more=true;z.promoPoint={z.morePoint.x+90,z.morePoint.y-115};break;}}}
 z.hud=anchor&&(companionMask==3||companionMask==5||companionMask==6||companionMask==7||(companionMask&&z.morePoint.x>0))&&!z.partial;
 if(anchor){double s=std::clamp(anchor->box.w/64.,.6,1.8);z.notice=notice(im,{anchor->box.center().x+int(7*s),anchor->box.y-int(120*s),int(70*s),int(72*s)});}
 return z;
}
enum class Action {Wait,OpenBuild,ClickBuy,Close,OpenMore,OpenPromo,Promote,Stop,ConfirmPromotion};
struct Step {Action action=Action::Wait;Point point{};std::wstring message;int delay=1500;};
inline bool clearHud(const Scene&s){return s.hud&&!s.build&&!s.promo&&!s.partial&&!s.more&&!s.moreOpen&&!s.confirmation&&!s.blocked;}
inline bool sameTarget(const Scene&before,const Scene&fresh,const Step&step){
 if(fresh.blocked)return false;
 auto nearby=[](Point a,Point b){return std::abs(a.x-b.x)<=8&&std::abs(a.y-b.y)<=8;};
 switch(step.action){
 case Action::OpenPromo:return fresh.more&&fresh.moreOpen&&nearby(fresh.promoPoint,step.point)&&nearby(before.morePoint,fresh.morePoint);
 // Hover enlarges this game's font: $700 may become "sno". The original
 // scene must have a numeric price; the fresh scene must still identify the
 // same building, level and enabled cash button. A readable changed price vetoes.
 case Action::ClickBuy:return before.available&&fresh.build&&fresh.buyEnabled&&nearby(fresh.buy,step.point)&&!before.name.empty()&&fresh.name==before.name&&fresh.level==before.level&&(!fresh.cost||!before.cost||fresh.cost==before.cost);
 case Action::Promote:return fresh.promo&&fresh.available&&nearby(fresh.buy,step.point)&&fresh.promotions==before.promotions&&fresh.cost==before.cost;
 case Action::Close:return fresh.build==before.build&&fresh.promo==before.promo&&(fresh.build||fresh.promo)&&!fresh.confirmation&&nearby(fresh.close,step.point);
 case Action::ConfirmPromotion:return fresh.confirmation&&nearby(fresh.buy,step.point);
 default:return false;
 }
}
class KeepAlive {
 double due=0;
public:
 explicit KeepAlive(double now):due(now+180){}
 bool ready(const Scene&s,double now)const{return now>=due&&clearHud(s);}
 void sent(double now){due=now+180;}
};
class Bot {
 enum class Phase {Watch,Opening,Buying,Closing,More,Promo,PromotionPending};
 Phase phase=Phase::Watch;int frames=0,retries=0,beforeLevel=-1,verifiedFrames=0,candidateCount=-1,unchangedBuys=0,lastFailedLevel=-1,confirmationFrames=0;std::wstring beforeName,lastFailedName;double candidateCost=-1,cooldown=0,nextPromo=0,pendingSince=0;int oldPromotions=-1;bool pending=false,confirmationSent=false;
 int moreAttempts=0,promoAttempts=0;double menuReadSince=-1,navigationSince=-1;
 bool recovering=false;double recoveryNext=0,unknownSince=-1,noPurchaseSince=-1;int recoveryClicks=0;
 std::wstring recoveryReason;
 Step recover(double now,const std::wstring&reason){
  if(pending)return {Action::Stop,{},reason+L" A promotion is pending; check its count."};
  resetNavigation();recovering=true;recoveryNext=now;recoveryClicks=0;recoveryReason=reason;recoveries++;
  return {Action::Wait,{},L"Recovering: "+reason,1500};
 }
 void resetNavigation(){moreAttempts=promoAttempts=0;menuReadSince=navigationSince=-1;}
public:
 bool autoPromote=true,saving=false;int purchases=0,promoted=0,confirmed=0,recoveries=0;std::wstring target;
 bool isRecovering()const{return recovering;}
 void retryNavigation(double now){if(!pending)recover(now,L"Input target changed. Rechecking the menu.");}
 bool promotionPending()const{return pending;}int expectedFrom()const{return oldPromotions;}
 bool confirmationAttempted()const{return confirmationSent;}
 void recoverPending(int count,double now,bool sent=false){pending=true;oldPromotions=count;pendingSince=now;confirmationSent=sent;phase=Phase::PromotionPending;}
 void focusLost(){resetNavigation();unknownSince=noPurchaseSince=-1;frames=retries=verifiedFrames=confirmationFrames=0;candidateCount=-1;phase=pending?Phase::PromotionPending:Phase::Watch;}
 void acknowledge(const Step&s,bool sent){if(!sent){if(s.action==Action::ConfirmPromotion)confirmationSent=false;focusLost();return;}if(s.action==Action::ClickBuy)purchases++;}
 Step next(const Scene&s,double now){
  auto wait=[](std::wstring m,int delay=500){return Step{Action::Wait,{},m,delay};};
  auto close=[&](std::wstring m){phase=Phase::Closing;retries=0;return Step{Action::Close,s.close,m,s.build?800:3000};};
  if(!s.promo){candidateCount=-1;verifiedFrames=0;}
  // No notification is only a reason to inspect readiness, never proof that
  // every building is complete. The promotion panel remains authoritative.
  if(clearHud(s)&&!s.notice){if(noPurchaseSince<0)noPurchaseSince=now;}
  else if(s.notice||s.build||s.promo||s.moreOpen||s.partial||s.confirmation||s.blocked)noPurchaseSince=-1;
  if(s.build||s.promo||s.moreOpen||clearHud(s))unknownSince=-1;
  if(s.blocked)return {Action::Stop,{},L"Premium purchase or disconnection detected. Please check Roblox."};
  if(pending&&now-pendingSince>90)return {Action::Stop,{},L"Promotion could not be verified. Check the game before continuing."};
  if(recovering){
   if(clearHud(s)){recovering=false;phase=Phase::Watch;resetNavigation();unknownSince=-1;frames=retries=0;cooldown=now+3;nextPromo=now+60;return wait(L"Menu recovered. Resuming the session.");}
   if(s.confirmation)return {Action::Stop,{},L"Unexpected confirmation during recovery. Check Roblox."};
   if(now<recoveryNext)return wait(L"Recovering: "+recoveryReason);
   recoveryNext=now+(++recoveryClicks<=3?5:60);
   if(s.build||s.promo)return {Action::Close,s.close,L"Recovery: closing the recognised panel.",3000};
   if(s.moreOpen&&s.morePoint.x>0)return {Action::OpenMore,s.morePoint,L"Recovery: closing the More menu.",3000};
   return wait(L"Needs attention: no safe menu target. Rechecking automatically.");
  }
  if(navigationSince>=0&&!s.promo&&!s.confirmation&&now-navigationSince>=60)return recover(now,L"Promotion navigation timed out.");
  if(s.confirmation){
   if(!pending)return {Action::Stop,{},L"Unexpected promotion confirmation. No recorded attempt; please check the game."};
   if(confirmationSent)return {Action::Stop,{},L"Promotion confirmation remains open. The confirmation will not be repeated."};
   if(++confirmationFrames<2)return wait(L"Promotion confirmation found. Checking one more reading...");
   confirmationSent=true;pendingSince=now;return {Action::ConfirmPromotion,s.buy,L"Confirming promotion once. Waiting for the new count.",8000};
  }
  confirmationFrames=0;
  if(s.promo){
   resetNavigation();
   if(pending){
    if(s.promotions==oldPromotions+1){if(++verifiedFrames<2)return wait(L"Promotion count increased. Confirming one more reading...");promoted++;pending=false;saving=false;nextPromo=now+90;cooldown=now+4;return close(L"Promotion verified. Resuming purchases.");}
    verifiedFrames=0;
    if(now-pendingSince<10)return wait(L"Waiting for the promotion result...");
    return {Action::Stop,{},L"The promotion count did not increase. Check the game; the click will not be repeated."};
   }
   if(phase==Phase::Closing){if(++retries>3)return recover(now,L"The promotion panel did not close.");return {Action::Close,s.close,L"Trying to close the promotion panel...",3000};}
   if(!autoPromote){nextPromo=now+60;return close(L"Auto-promotion is off.");}
   if(s.available){double price=s.cost.value_or(-1);if(candidateCount!=s.promotions||candidateCost!=price){candidateCount=s.promotions;candidateCost=price;return wait(L"Promotion is available. Confirming another reading...");}oldPromotions=s.promotions;pending=true;confirmationSent=false;pendingSince=now;retries=0;phase=Phase::PromotionPending;return {Action::Promote,s.buy,L"Promoting now. Waiting to verify the new count.",8000};}
   candidateCount=-1;frames=0;saving=s.ready;target=s.detail;nextPromo=now+(saving?60:600);
   return close(saving?L"Citadel complete. Saving for promotion; next check in 60 seconds.":L"Promotion is locked. Prioritising construction; next check no sooner than 10 minutes.");
  }
  if(s.partial){if(unknownSince<0)unknownSince=now;if(now-unknownSince>=30)return recover(now,L"The menu could not be read.");return wait(L"Waiting for a clear view of the open menu.");}
  if(s.more){
   if(navigationSince<0)navigationSince=now;
   menuReadSince=-1;phase=Phase::Promo;
   if(++promoAttempts>4)return recover(now,L"Promotion panel did not open after 4 clicks.");
   return {Action::OpenPromo,s.promoPoint,L"Opening the promotion panel...",3000};
  }
  if(s.moreOpen){
   if(navigationSince<0)navigationSince=now;
   if(menuReadSince<0)menuReadSince=now;
   if(now-menuReadSince>=30)return recover(now,L"Prestige could not be read for 30 seconds.");
   return wait(L"Reading the Prestige button in the More menu...");
  }
  if(s.build){
   if(pending)return close(L"Closing construction to verify the promotion.");
   if(phase==Phase::Closing){if(++retries>3)return recover(now,L"The construction panel did not close.");return {Action::Close,s.close,L"Trying the close button again...",800};}
   if(phase==Phase::Buying){if(s.name==beforeName&&beforeLevel>=0&&s.level==beforeLevel+1){confirmed++;unchangedBuys=0;return close(L"Upgrade verified. Refreshing the selection.");}if(++frames<3)return wait(L"Checking the purchase result...");if(s.name==beforeName&&s.level==beforeLevel){if(lastFailedName==s.name&&lastFailedLevel==s.level)unchangedBuys++;else unchangedBuys=1;lastFailedName=s.name;lastFailedLevel=s.level;if(unchangedBuys>=3)return {Action::Stop,{},L"Three purchases without progress. Check the selected building."};}cooldown=now+8;return close(L"Purchase not confirmed. Refreshing the selection.");}
   if(saving)return close(L"Saving this money for promotion.");
   if(!s.available||now<cooldown){cooldown=now+8;return close(L"No purchase is available. Closing the menu and waiting.");}
   beforeName=s.name;beforeLevel=s.level;frames=0;phase=Phase::Buying;return {Action::ClickBuy,s.buy,L"Buying: "+s.name,900};
  }
  if(!s.hud){if(unknownSince<0)unknownSince=now;if(now-unknownSince>=60)return recover(now,L"Game view has been unreadable for 60 seconds.");return wait(pending?L"Waiting for a clear view to verify the promotion.":L"Waiting for the game toolbar or a supported menu.");}
  unknownSince=-1;
  if(phase==Phase::Closing){phase=pending?Phase::PromotionPending:Phase::Watch;retries=frames=0;return wait(L"Menu closed.");}
  if(phase==Phase::Buying){phase=Phase::Watch;cooldown=now+.6;return wait(L"The game closed the purchase menu.");}
  bool navigationActive=phase==Phase::More||phase==Phase::Promo;
  bool constructionQuiet=noPurchaseSince>=0&&now-noPurchaseSince>=45;
  if(autoPromote&&(pending||navigationActive||(now>=nextPromo&&(saving||constructionQuiet)))){
   if(s.morePoint.x<=0)return wait(L"Looking for the More button to check promotion.");
   if(pending&&now-pendingSince<8)return wait(L"Waiting for the base to reset...");
   if(navigationSince<0)navigationSince=now;if(++moreAttempts>4)return recover(now,L"More menu did not open after 4 clicks.");phase=Phase::More;return {Action::OpenMore,s.morePoint,pending?L"Checking the promotion count...":L"Checking whether promotion is available...",3000};
  }
  if(saving)return wait(L"Saving for promotion. Checking every 60 seconds.");
  if(now<cooldown)return wait(L"Waiting for income before the next purchase.");
  if(phase==Phase::Opening){if(++retries>3)return recover(now,L"Construction did not respond to B.");return {Action::OpenBuild,{},L"Trying B again...",700};}
  if(!s.notice){frames=0;return wait(L"Waiting for a purchase notification.");}
  if(++frames<2)return wait(L"Purchase notification found. Confirming...");phase=Phase::Opening;retries=0;return {Action::OpenBuild,{},L"Opening construction with B...",700};
 }
};
}
