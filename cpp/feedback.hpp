#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>
namespace zzen {
inline std::string logTail(const std::filesystem::path& path,size_t limit=2*1024*1024){
 std::ifstream in(path,std::ios::binary);if(!in)return "(No log available.)\n";
 in.seekg(0,std::ios::end);auto size=in.tellg();if(size<0)return "(Log unavailable.)\n";
 const auto length=static_cast<size_t>(size);size_t offset=length>limit?length-limit:0;
 in.seekg(static_cast<std::streamoff>(offset));std::string text(length-offset,'\0');in.read(text.data(),static_cast<std::streamsize>(text.size()));text.resize(static_cast<size_t>(in.gcount()));
 if(offset){auto newline=text.find('\n');text=newline==std::string::npos?"":text.substr(newline+1);text="(Earlier entries omitted.)\n"+text;}return text;
}
inline void writeFeedback(const std::filesystem::path& path,const std::string& summary,const std::string& log){
 std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"Zzen 0.2.1 - Feedback log\nNative C++17 / Windows x64\nText only. No screenshots or attachments.\n\n"<<summary<<"\n--- Recent events ---\n"<<log;
 out.flush();if(!out)throw std::runtime_error("Could not save the feedback log. Check folder access and free space.");
}
}
