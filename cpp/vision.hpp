#pragma once
#define NOMINMAX
#include <windows.h>
#include <wincodec.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "core.hpp"
namespace zzen {
inline std::string utf8(const std::wstring&s){return winrt::to_string(s);}
inline Image loadImage(const std::filesystem::path&p){
 winrt::com_ptr<IWICImagingFactory> f;winrt::check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(f.put())));
 winrt::com_ptr<IWICBitmapDecoder>d;winrt::check_hresult(f->CreateDecoderFromFilename(p.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,d.put()));
 winrt::com_ptr<IWICBitmapFrameDecode>frame;winrt::check_hresult(d->GetFrame(0,frame.put()));winrt::com_ptr<IWICFormatConverter>c;winrt::check_hresult(f->CreateFormatConverter(c.put()));
 winrt::check_hresult(c->Initialize(frame.get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));UINT w,h;winrt::check_hresult(c->GetSize(&w,&h));Image im{int(w),int(h),std::vector<Pixel>(size_t(w)*h)};winrt::check_hresult(c->CopyPixels(nullptr,w*4,w*h*4,reinterpret_cast<BYTE*>(im.pixels.data())));return im;
}
inline void saveImage(const Image&im,const std::filesystem::path&p){
 BITMAPFILEHEADER f{};BITMAPINFOHEADER h{};h.biSize=sizeof(h);h.biWidth=im.w;h.biHeight=-im.h;h.biPlanes=1;h.biBitCount=32;f.bfType=0x4D42;f.bfOffBits=sizeof(f)+sizeof(h);f.bfSize=f.bfOffBits+DWORD(im.pixels.size()*4);
 std::ofstream out(p,std::ios::binary);out.write(reinterpret_cast<char*>(&f),sizeof(f));out.write(reinterpret_cast<char*>(&h),sizeof(h));out.write(reinterpret_cast<const char*>(im.pixels.data()),im.pixels.size()*4);
}
inline Image crop(const Image&im,Rect r,bool binary=false){
 r.x=std::max(0,r.x);r.y=std::max(0,r.y);r.w=std::min(r.w,im.w-r.x);r.h=std::min(r.h,im.h-r.y);if(r.w<1||r.h<1)return{};
 Image c{r.w*2,r.h*2,std::vector<Pixel>(size_t(r.w)*r.h*4)};bool yel=yellowRatio(im,r)>.45;
 static ULONG_PTR token=[](){Gdiplus::GdiplusStartupInput input;ULONG_PTR t;Gdiplus::GdiplusStartup(&t,&input,nullptr);return t;}();(void)token;
 Gdiplus::Bitmap source(im.w,im.h,im.w*4,PixelFormat32bppRGB,reinterpret_cast<BYTE*>(const_cast<Pixel*>(im.pixels.data())));
 Gdiplus::Bitmap dest(c.w,c.h,PixelFormat32bppARGB);
 {Gdiplus::Graphics g(&dest);g.DrawImage(&source,Gdiplus::Rect(0,0,c.w,c.h),r.x,r.y,r.w,r.h,Gdiplus::UnitPixel);}
 Gdiplus::BitmapData data;Gdiplus::Rect all(0,0,c.w,c.h);dest.LockBits(&all,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data);
 for(int y=0;y<c.h;y++)memcpy(c.pixels.data()+y*c.w,static_cast<BYTE*>(data.Scan0)+y*data.Stride,c.w*4);dest.UnlockBits(&data);
 if(binary)for(auto&p:c.pixels){bool ink=yel?(p.r<135&&p.g<135&&p.b<135):(p.r>180&&p.g>170);uint8_t v=ink?0:255;p={v,v,v,255};}return c;
}
class Reader {
 winrt::Windows::Media::Ocr::OcrEngine engine{nullptr};
public:
 Reader(){engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();if(!engine)throw std::runtime_error("Windows text recognition is not available.");}
 std::vector<Text> raw(const Image&im){
  if(im.w<=0||im.h<=0)return{};
  using namespace winrt::Windows::Graphics::Imaging;SoftwareBitmap bm(BitmapPixelFormat::Bgra8,im.w,im.h,BitmapAlphaMode::Ignore);
  auto ptr=reinterpret_cast<const uint8_t*>(im.pixels.data());bm.CopyFromBuffer(winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(winrt::array_view<const uint8_t>(ptr,ptr+im.pixels.size()*4)));
  auto result=engine.RecognizeAsync(bm).get();std::vector<Text> out;
  for(auto line:result.Lines()){double x=1e9,y=1e9,right=0,bottom=0;for(auto word:line.Words()){auto r=word.BoundingRect();x=std::min(x,double(r.X));y=std::min(y,double(r.Y));right=std::max(right,double(r.X+r.Width));bottom=std::max(bottom,double(r.Y+r.Height));}out.push_back({std::wstring(line.Text()),{int(x),int(y),int(right-x),int(bottom-y)}});}return out;
 }
 void addCrop(std::vector<Text>&out,const Image&im,Rect r,bool binary,bool replace=false){
  auto local=raw(crop(im,r,binary));for(auto&l:local){l.box={r.x+l.box.x/2,r.y+l.box.y/2,l.box.w/2,l.box.h/2};if(replace)out.erase(std::remove_if(out.begin(),out.end(),[&](auto&old){return r.contains(old.box.center())&&std::abs(old.box.center().y-l.box.center().y)<std::max(old.box.h,l.box.h)*.65;}),out.end());out.push_back(l);}
 }
 std::vector<Text> read(const Image&im){
  auto out=raw(im);
  if(im.w>=900&&im.h>=600){
   addCrop(out,im,{0,int(im.h*.92),int(im.w*.38),im.h-int(im.h*.92)},false);
   addCrop(out,im,{0,im.h-32,int(im.w*.38),32},false);
   addCrop(out,im,{0,im.h-32,int(im.w*.38),32},true);
   addCrop(out,im,{190,6,180,28},false,true);addCrop(out,im,{190,6,180,28},true);
   std::optional<Text> anchor;for(auto&t:out)if((norm(t.text)==L"construir"||norm(t.text)==L"consiruir")&&t.box.y>im.h*.9){anchor=t;break;}
   if(anchor){double scale=anchor->box.w/64.;for(int k=0;k<6;k++){int cx=anchor->box.center().x+int(k*106*scale);Rect label{std::max(0,cx-int(46*scale)),anchor->box.y-6,int(92*scale),anchor->box.h+12};addCrop(out,im,label,false);addCrop(out,im,label,true);}}
   if(auto p=moreDots(im)){
    Rect menu{std::max(0,p->x-250),p->y-180,500,135};addCrop(out,im,menu,false);
    // Isolate each popup label from its icon and the moving game background.
    for(int i=-2;i<=2;i++){Rect label{p->x+i*92-45,p->y-77,90,23};addCrop(out,im,label,false);addCrop(out,im,label,true);}
   }
  }
  if(auto b=buildLayout(out,im.w,im.h)){
   addCrop(out,im,b->area(910,325,405,85),true,true);
   for(auto r:{b->area(1060,695,240,65),b->area(1138,700,150,60)}){addCrop(out,im,r,false);addCrop(out,im,r,true);}
  }
  if(auto p=promoLayout(out,im.w,im.h)){
   addCrop(out,im,p->area(25,130,710,70),false,true);addCrop(out,im,p->area(302,505,210,48),false);addCrop(out,im,p->area(302,505,210,48),true);
  }
  return out;
 }
};
inline std::string report(const Scene&s,const std::vector<Text>&lines){
 std::ostringstream o;o<<"build="<<s.build<<" promo="<<s.promo<<" hud="<<s.hud<<" partial="<<s.partial<<" more="<<s.more<<" notice="<<s.notice<<" available="<<s.available<<" ready="<<s.ready<<" promotions="<<s.promotions<<" balance="<<(s.balance?*s.balance:-1)<<" cost="<<(s.cost?*s.cost:-1)<<" layout="<<s.layout.x<<","<<s.layout.y<<","<<s.layout.s<<"\n";
 for(auto&t:lines)o<<t.box.x<<","<<t.box.y<<","<<t.box.w<<","<<t.box.h<<": "<<utf8(t.text)<<"\n";return o.str();
}
}
