#include <iostream>
#include "nozzle.hpp"

int main(const int argc, const char** argv)
{
	if (argc < 2)
	{
		std::perror("[-] please provide a dll path...");
		return -1;
	}

	if (!util::get_pid(L"BEService.exe"))
	{
		std::perror("[-] please run BattlEye...\n");
		return -1;
	}

	const auto lsass_pid = util::get_pid(L"lsass.exe");
	std::printf("[+] lsass_pid => %p\n", lsass_pid);

	nozzle::injector inject(argv[1], lsass_pid);
	const auto module_base = inject.inject();
	inject.hook_entry();

	std::printf("[+] module base => %p\n", module_base);
	std::getchar();
}