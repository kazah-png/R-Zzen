#include "vision.hpp"
#include "feedback.hpp"
#include <iostream>
using namespace zzen;
int checks=0,fail=0,skippedFixtures=0;
bool requireFixtures=false;
bool fixtureAvailable(const std::filesystem::path& path){
 if(std::filesystem::exists(path))return true;
 skippedFixtures++;
 if(requireFixtures)throw std::runtime_error("Required private fixture is missing: "+path.string());
 std::cout<<"SKIP private fixture: "<<path.string()<<"\n";
 return false;
}
void check(bool value,const char*name){checks++;if(!value){std::cerr<<"FAIL "<<name<<"\n";fail++;}}
Scene promo(bool enabled=false){Scene s;s.promo=true;s.ready=true;s.promotions=11;s.available=enabled;s.buy={960,800};s.close={1298,309};return s;}
Scene hud(){Scene s;s.hud=true;s.notice=true;s.morePoint={517,1005};return s;}
int wmain(int argc,wchar_t**argv){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);
 try{
 requireFixtures=argc>1&&std::wstring(argv[1])==L"--require-fixtures"; if(argc>1&&!requireFixtures){Reader reader;auto im=loadImage(argv[1]);auto text=reader.read(im);auto scene=analyze(im,text);auto output=report(scene,text);std::cout<<output;if(argc>2){std::ofstream out(argv[2]);out<<output;}return 0;}
 check(money(L"PROMOCIONAR · $22.0B")==22e9,"promotion cost");check(money(L"$17,303,980,458")==17303980458.,"balance");check(money(L"Mejora $36.0M")==36e6,"buy cost");check(!money(L"Necesita fortaleza"),"no price");
 {Bot b;b.saving=true;check(b.next(hud(),0).action==Action::OpenMore,"known-ready promotion inspection");Scene m;m.more=true;check(b.next(m,3).action==Action::OpenPromo,"more -> promotion");check(b.next(promo(),6).action==Action::Close,"gray promotion not clicked");check(b.saving,"saves for ready citadel");b.next(hud(),9);check(b.next(hud(),12).action==Action::Wait,"does not spend savings");check(b.next(hud(),67).action==Action::OpenMore,"rechecks after minute");}
 {Bot b;auto p=promo(true);check(b.next(p,0).action==Action::Wait,"two reads before promotion");check(b.next(p,2).action==Action::Promote,"enabled promotes");check(b.promotionPending(),"pending persisted by runtime");check(b.next(p,5).action==Action::Wait,"no duplicate click");check(b.next(p,14).action==Action::Stop,"unchanged count stops");}
 {Bot b;auto p=promo(true);b.next(p,0);b.next(p,2);p.promotions=12;p.available=false;check(b.next(p,10).action==Action::Wait,"verification requires two reads");check(b.next(p,12).action==Action::Close,"increased count confirmed");check(!b.promotionPending()&&b.promoted==1&&!b.saving,"savings released");b.next(hud(),15);b.next(hud(),18);check(b.next(hud(),20).action==Action::OpenBuild,"buying resumes after promotion");Scene build;build.build=true;build.available=true;build.level=1;build.name=L"petroleo";check(b.next(build,23).action==Action::ClickBuy,"postpromotion purchase");build.level=2;check(b.next(build,26).action==Action::Close&&b.confirmed==1,"purchase level verified");}
 {Bot b;b.recoverPending(11,0);check(b.next(hud(),10).action==Action::OpenMore,"resume checks old transaction");auto p=promo(true);p.promotions=12;b.next(p,15);check(b.next(p,17).action==Action::Close,"recovered transaction confirmed");}
 {Bot b;b.recoverPending(11,0);check(b.next(hud(),91).action==Action::Stop,"unknown transaction timeout");}
 {Bot b;b.autoPromote=false;auto p=promo(true);check(b.next(p,0).action==Action::Close&&!b.promotionPending(),"option off");}
 {Bot b;auto p=promo();b.next(p,0);for(int i=1;i<=3;i++)check(b.next(p,i*3).action==Action::Close,"close retry");check(b.next(p,12).action==Action::Wait&&b.isRecovering(),"close retry starts bounded recovery");}
 {Bot b;Scene s;s.blocked=true;check(b.next(s,0).action==Action::Stop,"premium blocked");}
 {Bot b;auto p=promo(true);b.next(p,0);b.next(p,2);b.focusLost();p.promotions=12;b.next(p,15);check(b.next(p,17).action==Action::Close,"focus loss preserves promotion");}
 {Bot b;auto p=promo();p.ready=false;b.next(p,0);check(!b.saving,"not ready continues building");}
 {Bot b;check(b.next(hud(),0).action==Action::Wait,"startup validates purchase notice before input");
  check(b.next(hud(),.5).action==Action::OpenBuild,"startup prioritises construction over prestige");}
 {Bot b;auto quiet=hud();quiet.notice=false;
  check(b.next(quiet,0).action==Action::Wait&&b.next(quiet,44).action==Action::Wait,"quiet HUD does not immediately imply completion");
  Scene unknown;b.next(unknown,44.5);
  check(b.next(quiet,45).action==Action::OpenMore,"transient OCR gap does not starve readiness inspection");
  auto locked=promo();locked.ready=false;
  check(b.next(locked,48).action==Action::Close&&!b.saving&&!b.promotionPending(),"incomplete buildings never enable prestige or saving");
  b.next(quiet,51);b.next(quiet,100);
  check(b.next(quiet,647).action==Action::Wait,"locked prestige stays deferred for ten minutes");
  check(b.next(quiet,648).action==Action::OpenMore,"locked prestige can recheck after ten quiet minutes");}
 {Bot b;auto quiet=hud();quiet.notice=false;b.next(quiet,0);b.next(hud(),40);
  check(b.next(hud(),40.5).action==Action::OpenBuild,"purchase notice interrupts quiet-period observation");
  b.focusLost();b.next(quiet,80);
  check(b.next(quiet,124).action==Action::Wait&&b.next(quiet,125).action==Action::OpenMore,"focus pause requires fresh quiet-period observation");}
 {Bot b;Scene build;build.build=true;build.available=true;build.name=L"oil";build.level=0;
  double now=0;bool priority=true;int buys=0;
  // Simulate 200 fully observed purchases, including both menu behaviours.
  // Account for one second of OCR per observation and a second fresh read per click.
  auto step=[&](Scene scene){auto result=b.next(scene,now);now+=1+result.delay/1000.;if(result.action!=Action::Wait)now+=1;priority&=result.action!=Action::OpenMore&&result.action!=Action::OpenPromo&&result.action!=Action::Stop;return result;};
  for(int i=0;i<200;i++){
   priority&=step(hud()).action==Action::Wait;priority&=step(hud()).action==Action::OpenBuild;
   auto buy=step(build);priority&=buy.action==Action::ClickBuy;b.acknowledge(buy,true);buys++;
   if(i%2==0){build.level++;priority&=step(build).action==Action::Close;priority&=step(hud()).action==Action::Wait;}
   else{priority&=step(hud()).action==Action::Wait;build.level++;}
  }
  check(priority&&buys==200&&b.purchases==200,"200 continuous purchases never interrupt construction for prestige");
  check(now/200<12,"verified purchase cycle stays under twelve seconds with simulated OCR cost");}
 Image gray{1920,1080,std::vector<Pixel>(1920*1080,Pixel{150,150,150,255})};
 std::vector<Text> text={{L"PROMOCIÓN",{622,282,120,25}},{L"Renacimiento: empezar de nuevo más fuerte",{615,320,500,20}},{L"Promociones hasta ahora: 11. Ciudadela de comandos construida.",{615,420,680,20}},{L"Ahorra $22.0B (tienes $17.4B), luego presiona PROMOCIONAR.",{615,444,680,20}},{L"PROMOCIONAR · $22.0B",{850,790,240,25}},{L"$17,400,000,000",{230,12,130,20}}};
 auto s=analyze(gray,text);check(s.promo&&s.promotions==11&&s.ready&&!s.available,"gray promotion image geometry");
 for(int y=762;y<838;y++)for(int x=796;x<1124;x++)gray.pixels[y*1920+x]={0,240,255,255};
 check(!analyze(gray,text).available,"insufficient balance veto");text.back().text=L"$22,400,000,000";text[3].text=L"Ahorra $22.0B (tienes $22.4B), luego presiona PROMOCIONAR.";check(analyze(gray,text).available,"yellow affordable promotion");
 text[2].text=L"Promociones hasta ahora: 11. Todo está listo: presiona PROMOCIONAR.";
 check(analyze(gray,text).available,"ready message replaces citadel requirement when affordable");
 text[2].text=L"Promociones hasta ahora: 11. Necesita ciudadela. Todo está listo: presiona PROMOCIONAR.";
 check(!analyze(gray,text).available,"requirement vetoes contradictory ready message");
 text[2].text=L"Promotions so far: 12. How: 1. Upgrade your HQ to the Command Citadel (HQ level 5 - yours is level 1). 2. Pay $28.0B here.";
 check(analyze(gray,text).promo&&analyze(gray,text).promotions==12&&!analyze(gray,text).ready,"mixed-language counter after rebirth");
 text.erase(text.begin()+2);check(!analyze(gray,text).available,"missing counter never promotes");
 check(!money(L"$22.DB"),"reject incomplete OCR price");check(!money(L"$57,66u52"),"reject incomplete OCR balance");
 {Scene c;c.confirmation=true;c.buy={1116,680};Bot b;
  check(b.next(c,0).action==Action::Stop,"unknown confirmation cannot authorize rebirth");
  b.recoverPending(11,0);check(b.next(c,1).action==Action::Wait,"confirmation requires two readings");
  b.focusLost();check(b.next(c,2).action==Action::Wait,"focus loss clears confirmation reading");
  check(b.next(c,4).action==Action::ConfirmPromotion&&b.confirmationAttempted(),"recorded attempt confirms once");
  check(b.next(c,12).action==Action::Stop,"confirmation never repeated");
  b.acknowledge({Action::ConfirmPromotion},false);check(!b.confirmationAttempted(),"pre-input cancellation clears sent flag");
  check(b.next(c,13).action==Action::Wait&&b.next(c,15).action==Action::ConfirmPromotion,"cancelled input rechecks before retry");
  Bot recovered;recovered.recoverPending(11,20,true);check(recovered.next(c,21).action==Action::Stop,"restart preserves sent confirmation");
  auto result=promo();result.promotions=12;recovered.next(result,22);check(recovered.next(result,24).action==Action::Close&&recovered.promoted==1,"confirmed transaction still verifies counter");}
 {Bot b;Scene menu;menu.hud=true;menu.moreOpen=true;menu.morePoint={517,1005};
  b.next(hud(),0);for(int i=1;i<=3;i++)b.next(menu,i*2);
  menu.more=true;menu.promoPoint={607,889};
  check(b.next(menu,8).action==Action::OpenPromo,"regression: OCR retries cannot exhaust first promotion click");}
 Reader reader;
 if(fixtureAvailable(L"fixtures/v02-promo-hover.bmp")){auto im=loadImage(L"fixtures/v02-promo-hover.bmp");auto fresh=analyze(im,reader.read(im));Scene before=fresh;before.promoPoint={607,888};Step step{Action::OpenPromo,before.promoPoint};check(sameTarget(before,fresh,step),"hover font height must not shift the promotion icon target");}
 {Bot b;Scene menu;menu.moreOpen=true;menu.more=true;menu.morePoint={517,1005};menu.promoPoint={607,890};
  for(int i=0;i<4;i++)b.next(menu,i*3);
  check(b.next(menu,12).action==Action::Wait&&b.isRecovering(),"menu failure preserves a recoverable session");
  check(b.next(menu,14).action==Action::OpenMore,"recovery closes only recognised More menu");
  check(b.next(hud(),18).action==Action::Wait&&!b.isRecovering(),"clear HUD ends recovery");
  b.next(hud(),22);check(b.next(hud(),24).action==Action::OpenBuild,"construction resumes after navigation recovery");
  b.focusLost();b.saving=true;check(b.next(hud(),80).action==Action::OpenMore,"known-ready promotion check resumes after backoff");}
 {KeepAlive active(0);auto clean=hud();check(!active.ready(clean,179)&&active.ready(clean,180),"keep-active has an independent three-minute deadline");
  for(int kind=0;kind<6;kind++){auto menu=clean;if(kind==0)menu.moreOpen=true;if(kind==1)menu.partial=true;if(kind==2)menu.confirmation=true;if(kind==3)menu.blocked=true;if(kind==4)menu.build=true;if(kind==5)menu.promo=true;check(!active.ready(menu,300),"keep-active must not type into a menu or blocked view");}
  active.sent(180);check(!active.ready(clean,359)&&active.ready(clean,360),"successful keep-active resets its own deadline");}
 {Scene before;before.moreOpen=true;before.more=true;before.morePoint={609,1005};before.promoPoint={698,890};Step click{Action::OpenPromo,before.promoPoint};
  check(sameTarget(before,before,click),"stable promotion popup target is valid");auto changed=before;changed.morePoint.x-=92;changed.promoPoint.x-=92;check(!sameTarget(before,changed,click),"day/night shift cancels promotion click before input");
  changed=before;changed.moreOpen=false;check(!sameTarget(before,changed,click),"popup closing cancels stale promotion target");}
 {Bot b;KeepAlive active(0);int jumps=0,cycles=0;bool safe=true;Scene menu;menu.more=true;menu.moreOpen=true;menu.morePoint={517,1005};menu.promoPoint={607,890};
  // Four virtual hours, with a failed promotion-menu navigation every cycle.
  for(int second=0;second<4*60*60;second+=120){
   for(int k=0;k<5;k++)safe&=b.next(menu,second+k*3).action!=Action::Stop;
   safe&=b.isRecovering();safe&=b.next(menu,second+16).action==Action::OpenMore;
   b.next(hud(),second+20);cycles++;
   for(int k=21;k<120;k++){if(active.ready(hud(),second+k)){active.sent(second+k);jumps++;}}
  }
  check(safe&&cycles==120&&b.recoveries==120,"four-hour simulation survives 120 repeated menu failures");
  check(jumps>=70,"four-hour recovery simulation retains keep-active scheduling");}
 {Reader cancelled([]{return true;});bool interrupted=false;try{cancelled.read(gray);}catch(const std::exception&){interrupted=true;}check(interrupted,"stopped session cancels OCR before waiting for Windows");}
 if(fixtureAvailable(L"fixtures/v02-stuck-build.bmp")){auto im=loadImage(L"fixtures/v02-stuck-build.bmp");auto sc=analyze(im,reader.read(im));check(sc.build&&sc.available&&sc.level==0&&has(sc.name,L"petrolera 5"),"v02 regression: BUILD $700 must not stall for hours as BUILD sno");}
 if(fixtureAvailable(L"fixtures/v02-hover-build.bmp")&&fixtureAvailable(L"fixtures/v02-stuck-build.bmp")){
  auto im=loadImage(L"fixtures/v02-stuck-build.bmp"),hover=loadImage(L"fixtures/v02-hover-build.bmp");auto before=analyze(im,reader.read(im)),fresh=analyze(hover,reader.read(hover));Step step{Action::ClickBuy,before.buy};
  check(sameTarget(before,fresh,step),"real hover font change preserves verified numeric-price purchase");
  fresh.buyEnabled=false;check(!sameTarget(before,fresh,step),"disabled hover button cancels purchase");fresh.buyEnabled=true;
  fresh.name=L"another building";check(!sameTarget(before,fresh,step),"changed selected building cancels purchase");fresh.name=before.name;
  fresh.cost=12345;check(!sameTarget(before,fresh,step),"readable changed price cancels purchase");
 }
 if(fixtureAvailable(L"fixtures/native-after-rebirth-upgrade.bmp")){auto im=loadImage(L"fixtures/native-after-rebirth-upgrade.bmp");auto sc=analyze(im,reader.read(im));check(sc.build&&sc.available&&sc.level==1,"real UPGRADE and NIVELI recognition");}
 if(fixtureAvailable(L"fixtures/native-after-rebirth-build.bmp")){auto im=loadImage(L"fixtures/native-after-rebirth-build.bmp");auto sc=analyze(im,reader.read(im));check(sc.build&&sc.available&&sc.level==0,"real rebirth first BUILD purchase recognised");}
 if(fixtureAvailable(L"fixtures/native-promo-count12.bmp")){auto im=loadImage(L"fixtures/native-promo-count12.bmp");auto sc=analyze(im,reader.read(im));check(sc.promo&&sc.promotions==12&&!sc.available,"real mixed-language promotion count 12");}
 if(fixtureAvailable(L"fixtures/native-more-rebirth-night.bmp")){auto im=loadImage(L"fixtures/native-more-rebirth-night.bmp");auto sc=analyze(im,reader.read(im));check(sc.moreOpen&&sc.more&&abs(sc.promoPoint.x-698)<8,"rebirth night popup guarded OCR variant");}
 if(fixtureAvailable(L"fixtures/native-promo-confirm.bmp")){auto im=loadImage(L"fixtures/native-promo-confirm.bmp");auto lines=reader.read(im);auto sc=analyze(im,lines);check(sc.confirmation&&abs(sc.buy.x-1116)<5&&abs(sc.buy.y-680)<5,"real promotion confirmation target");
  lines.erase(std::remove_if(lines.begin(),lines.end(),[](auto&l){return norm(l.text)==L"cancelar";}),lines.end());check(!analyze(im,lines).confirmation,"modal requires cancel and promote controls");}
 if(fixtureAvailable(L"fixtures/native-promo-ready.bmp")){auto im=loadImage(L"fixtures/native-promo-ready.bmp");auto sc=analyze(im,reader.read(im));check(sc.promo&&sc.ready&&sc.available&&sc.promotions==11&&sc.cost==22e9,"real affordable promotion ready message");}
 for(auto path:{L"fixtures/purchase.png",L"fixtures/live-hover-button.png",L"fixtures/live-missed-button.png",L"fixtures/live-price-10m.png"}){if(!fixtureAvailable(path))continue;auto im=loadImage(path);auto t=reader.read(im);auto sc=analyze(im,t);check(sc.build&&sc.available,"real purchase regression");}
 if(fixtureAvailable(L"fixtures/blocked.png")){auto im=loadImage(L"fixtures/blocked.png");check(!analyze(im,reader.read(im)).available,"real gray buy");}
 if(fixtureAvailable(L"fixtures/native-promo-gray.bmp")){auto im=loadImage(L"fixtures/native-promo-gray.bmp");auto sc=analyze(im,reader.read(im));check(sc.promo&&!sc.available&&sc.ready&&sc.promotions==11&&sc.cost==22e9,"live promotion capture");}
 for(auto path:{L"fixtures/native-hud.bmp",L"fixtures/live-hud-daylight.png",L"fixtures/native-hud-bright.bmp"}){if(!fixtureAvailable(path))continue;auto im=loadImage(path);auto p=moreDots(im);check(p&&p->x>400&&p->y>950,"real More icon");check(analyze(im,reader.read(im)).hud,"real HUD recognition");}
 check(!moreDots(gray),"no ellipsis in blank screen");
 {Bot b;auto p=promo(true);p.cost=22e9;b.next(p,0);p.cost=24e9;check(b.next(p,2).action==Action::Wait,"changed price needs another read");check(b.next(p,4).action==Action::Promote,"stable price and count promote");}
 {Bot b;auto p=promo(true);b.next(p,0);b.focusLost();check(b.next(p,3).action==Action::Wait,"focus loss clears candidate");check(b.next(p,5).action==Action::Promote,"candidate confirmed after resume");}
 {Bot b;b.autoPromote=false;Scene s;s.build=true;s.available=true;s.name=L"oil";s.level=1;double t=0;Step step;for(int i=0;i<3;i++){b.next(s,t);b.next(s,t+3);b.next(s,t+5);step=b.next(s,t+7);if(i<2){b.next(hud(),t+10);t+=20;}}check(step.action==Action::Stop,"three unchanged purchases stop");}
 {Bot b;Scene menu;menu.hud=true;menu.moreOpen=true;menu.morePoint={517,1005};for(int i=0;i<4;i++)check(b.next(menu,i*3).action==Action::Wait,"unreadable open menu never toggles More");check(b.next(menu,30).action==Action::Wait&&b.isRecovering(),"unreadable menu recovers after 30 seconds");}
 {Image blank{1920,1080,std::vector<Pixel>(1920*1080)};std::vector<Text> labels={{L"CONSTRUIR",{30,1050,60,14}},{L"EJERCITO",{130,1050,60,14}},{L"MAS",{510,1050,24,14}}};check(analyze(blank,labels).morePoint.x==0,"More label alone cannot authorize a click");}
 if(fixtureAvailable(L"fixtures/native-more-day.bmp")){auto im=loadImage(L"fixtures/native-more-day.bmp");auto sc=analyze(im,reader.read(im));check(sc.more&&sc.moreOpen&&sc.promoPoint.x>590&&sc.promoPoint.x<620,"real popup isolated label recognition");}
 {auto path=std::filesystem::path(L"native/feedback-test.txt");writeFeedback(path,"Status: test\n","one\ntwo\nthree\n");auto tail=logTail(path,10);check(tail.find("Earlier entries omitted")!=std::string::npos&&tail.find("three")!=std::string::npos,"feedback tail bounded and complete lines");auto full=logTail(path);check(full.find("Text only")!=std::string::npos&&full.find("Status: test")!=std::string::npos,"feedback report metadata");check(logTail(L"native/absent-log-test").find("No log")!=std::string::npos,"missing log export supported");}
 if(fixtureAvailable(L"fixtures/native-more-hover.bmp")){auto im=loadImage(L"fixtures/native-more-hover.bmp");auto dots=moreDots(im);check(dots&&abs(dots->x-609)<=3&&abs(dots->y-1005)<=3,"hover gradient preserves More dots");auto sc=analyze(im,reader.read(im));check(sc.hud&&!sc.moreOpen&&!sc.more,"More tooltip is not the popup");}
 if(fixtureAvailable(L"fixtures/native-more-night.bmp")){auto im=loadImage(L"fixtures/native-more-night.bmp");auto sc=analyze(im,reader.read(im));check(sc.more&&sc.moreOpen&&sc.promoPoint.x>680&&sc.promoPoint.x<715,"popup OCR typo requires verified popup and correct column");}
 {Bot b;Scene menu;menu.moreOpen=true;menu.hud=true;menu.morePoint={517,1005};
  b.next(hud(),0);for(int i=1;i<=12;i++)check(b.next(menu,i*2).action==Action::Wait,"many OCR readings do not count as clicks");
  menu.more=true;menu.promoPoint={607,889};
  for(int i=0;i<4;i++)check(b.next(menu,26+i*3).action==Action::OpenPromo,"promotion has independent four-click budget");
  check(b.next(menu,39).action==Action::Wait&&b.isRecovering(),"four failed promotion clicks enter recovery");}
 {Bot b;b.saving=true;for(int i=0;i<4;i++)check(b.next(hud(),i*3).action==Action::OpenMore,"More has independent four-click budget");
  check(b.next(hud(),12).action==Action::Wait&&b.isRecovering(),"four failed More clicks enter recovery");}
 {Bot b;b.saving=true;b.next(hud(),0);Scene unknown;
  check(b.next(unknown,59).action==Action::Wait,"transient unknown navigation waits");
  check(b.next(unknown,60).action==Action::Wait&&b.isRecovering(),"unrecognised navigation enters visible recovery");}
 {Bot b;b.saving=true;Scene menu;menu.moreOpen=true;menu.more=true;menu.promoPoint={607,889};bool ok=true;
  for(int i=0;i<1000;i++){double n=i*75.;ok&=b.next(hud(),n).action==Action::OpenMore;
   ok&=b.next(menu,n+3).action==Action::OpenPromo;ok&=b.next(promo(),n+6).action==Action::Close;
   ok&=b.next(hud(),n+9).action==Action::Wait;}
  check(ok,"1000 promotion checks reset navigation budgets after real progress");}
 {Bot b;Scene menu;menu.moreOpen=true;menu.hud=true;menu.morePoint={517,1005};b.next(menu,0);b.focusLost();
  check(b.next(menu,40).action==Action::Wait,"focus pause resets menu reading timeout");}
 {Image popup{1920,1080,std::vector<Pixel>(1920*1080,Pixel{90,70,60,255})};
  for(int cx:{496,517,538})for(int y=1001;y<=1009;y++)for(int x=cx-4;x<=cx+4;x++)popup.pixels[y*1920+x]={255,255,255,255};
  std::vector<Text> labels={{L"CONSTRUIR",{30,1050,64,14}},{L"EJERCITO",{130,1050,60,14}},
   {L"MAPA",{319,935,26,10}},{L"EQUIPAMIENTO",{390,935,69,12}},{L"",{572,935,71,10}}};
  for(auto variant:{L"PRECONAR",L"PRECIONAR",L"PRO\"CIONAR",L"PRECKNAR"}){
   labels.back().text=variant;auto sc=analyze(popup,labels);check(sc.moreOpen&&sc.more&&abs(sc.promoPoint.x-607)<3,"guarded OCR variants recognised in verified popup");
   labels.back().box.x=400;check(!analyze(popup,labels).more,"OCR typo outside promotion column is rejected");labels.back().box.x=572;
  }
  labels.back().text=L"PROMOCIONAR";labels.back().box={572,934,71,10};auto shortText=analyze(popup,labels);labels.back().box.h=14;auto tallText=analyze(popup,labels);check(sameTarget(shortText,tallText,{Action::OpenPromo,shortText.promoPoint})&&shortText.promoPoint.y==890,"promotion icon position is independent of hover text height");
  labels.erase(labels.begin()+2,labels.begin()+4);check(!analyze(popup,labels).more,"OCR variants alone cannot authorize a popup click");}
 if(fixtureAvailable(L"fixtures/native-more-stop-110808.bmp")){auto im=loadImage(L"fixtures/native-more-stop-110808.bmp");auto sc=analyze(im,reader.read(im));check(sc.moreOpen&&sc.more&&abs(sc.promoPoint.x-607)<8,"actual stopped menu is recognised");}
 std::cout<<checks<<" tests; "<<fail<<" failures; "<<skippedFixtures<<" private fixture files skipped\n";return fail?1:0;
 }catch(const winrt::hresult_error&e){std::cerr<<winrt::to_string(e.message())<<"\n";return 2;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 2;}
}
