#include "../../src/main/legacy_save_import.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
namespace fs=std::filesystem;
using sssv::LegacySaveImport;
static void put(const fs::path& p,const std::string& b){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary).write(b.data(),b.size());}
static std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
const auto root=fs::temp_directory_path()/ ("sssv-save-import-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
const fs::path filename="sssv.n64.us.1.0.bin";const auto source=root/"legacy"/filename;const std::string old(512,'L'),current(512,'P');put(source,old);const auto source_time=fs::last_write_time(source);
auto run=[&](const char* target){return sssv::import_legacy_save(root/target,root/"legacy",filename);};
assert(run("missing-folder")==LegacySaveImport::Imported);assert(read(root/"missing-folder"/filename)==old);
fs::create_directories(root/"empty-folder");assert(run("empty-folder")==LegacySaveImport::Imported);
put(root/"existing"/filename,current);assert(run("existing")==LegacySaveImport::ExistingPortableSave);assert(read(root/"existing"/filename)==current);
put(root/"backup"/(filename.string()+".bak"),current);assert(run("backup")==LegacySaveImport::ExistingPortableSave);assert(!fs::exists(root/"backup"/filename));
put(root/"bank"/"another-bank.bin",current);assert(run("bank")==LegacySaveImport::ExistingPortableSave);
put(root/"nested"/"mod"/"slot.bin.bak",current);assert(run("nested")==LegacySaveImport::ExistingPortableSave);
put(root/"temp"/"slot.bin.temp",current);assert(run("temp")==LegacySaveImport::ExistingPortableSave);
put(root/"notes"/"README.txt","not savedata");assert(run("notes")==LegacySaveImport::Imported);
put(root/"zero-existing"/filename,"");assert(run("zero-existing")==LegacySaveImport::ExistingPortableSave);assert(fs::file_size(root/"zero-existing"/filename)==0);
assert(read(source)==old);assert(fs::last_write_time(source)==source_time);
put(root/"wrong-legacy"/"unrelated.bin",old);put(root/"wrong-legacy"/"README.txt",old);assert(sssv::import_legacy_save(root/"no-import",root/"wrong-legacy",filename)==LegacySaveImport::NoLegacySave);assert(!fs::exists(root/"no-import"/filename));
put(root/"bad-size"/filename,"wrong");assert(sssv::import_legacy_save(root/"bad-size-target",root/"bad-size",filename)==LegacySaveImport::InvalidLegacySave);assert(!fs::exists(root/"bad-size-target"/filename));
assert(sssv::import_legacy_save(root/"nothing",root/"missing-legacy",filename)==LegacySaveImport::NoLegacySave);
assert(read(source)==old);assert(fs::weakly_canonical(root).parent_path()==fs::weakly_canonical(fs::temp_directory_path()));fs::remove_all(root);std::cout<<"Legacy import checks passed: absent/empty folder import, existing save/backup/bank/temp preserved, original untouched, wrong name/size ignored.\n";
}
