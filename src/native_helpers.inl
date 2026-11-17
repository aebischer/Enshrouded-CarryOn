// Helpers shared by the adapters; UTF-8 bytes follow the native signed-byte hash.
constexpr bool TransportDiagnostics=false;
uint64_t nativeTextHash(const char* text) {
 uint64_t value=0xcbf29ce484222325ull;
 for(auto p=text;*p;++p)value=(value^static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(*p))))*0x100000001b3ull;
 return value;
}
bool gameHasFocus() {
 DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
 return process==GetCurrentProcessId();
}

namespace transport_time {
constexpr ULONGLONG SelectionFresh=1000,InputFresh=250,GrabLifetime=1000;
constexpr ULONGLONG PreviewStartup=2000,PreviewLifetime=1000,CommitLifetime=1000,SyncWarning=5000;
constexpr ULONGLONG UiFresh=5000,SyncRetry=1000;
constexpr bool within(ULONGLONG now,ULONGLONG stamp,ULONGLONG limit) {
 return now>=stamp&&now-stamp<=limit;
}
constexpr bool fresh(ULONGLONG now,ULONGLONG stamp,ULONGLONG limit) {
 return stamp&&within(now,stamp,limit);
}
static_assert(!fresh(100,0,100)&&!fresh(99,100,100));
static_assert(fresh(350,100,InputFresh)&&!fresh(351,100,InputFresh));
}
