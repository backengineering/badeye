#include "utils.hpp"

void run_example()
{
	OutputDebugStringA("[lsass] main thread created!");

	const auto proc_handle =
		OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, utils::get_pid(L"notepad.exe"));

	if (proc_handle == INVALID_HANDLE_VALUE)
	{
		OutputDebugStringA("[lsass] failed to open handle to system process...");
		return;
	}

	const auto ntdll_base =
		reinterpret_cast<std::uintptr_t>(GetModuleHandleA("ntdll.dll"));

	if (bedaisy::read<short>(proc_handle, ntdll_base) == IMAGE_DOS_SIGNATURE)
		OutputDebugStringA("[lsass] read ntdll MZ from notepad.exe using BEDaisy.sys...\n");
	else
		OutputDebugStringA("[lsass] failed to read MZ...\n");
}

std::atomic<bool> init = false;
extern "C" auto nt_close(void* handle) -> NTSTATUS
{
	if (!init.exchange(true))
	{
		OutputDebugStringA("[lsass] creating thread!");
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&run_example, NULL, NULL, NULL);
	}
	return NULL;
}