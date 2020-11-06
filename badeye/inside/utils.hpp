#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <string>
#include "bedaisy.hpp"

namespace utils
{
	typedef struct _nt_peb
	{
		std::uintptr_t res[2];
		std::uintptr_t image_base;
		std::uintptr_t ldr;
		std::uintptr_t proc_params;
	} nt_peb;

	__forceinline auto get_pid(const std::wstring_view process_name) -> std::uint32_t
	{
		const auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return !CloseHandle(handle);

		PROCESSENTRY32W process_entry{ sizeof(PROCESSENTRY32W) };
		for (Process32FirstW(handle, &process_entry); Process32NextW(handle, &process_entry); )
		{
			if (std::wcscmp(process_name.data(), process_entry.szExeFile) == NULL)
			{
				CloseHandle(handle);
				return process_entry.th32ProcessID;
			}
		}
		CloseHandle(handle);
		return NULL;
	}

	__forceinline auto get_process_peb(const HANDLE process_handle) -> nt_peb*
	{
		PROCESS_BASIC_INFORMATION process_info{};
		ULONG bytes_returned;
		if (NtQueryInformationProcess
		(
			process_handle,
			ProcessBasicInformation,
			&process_info,
			sizeof(process_info),
			&bytes_returned
		) != ERROR_SUCCESS)
			return nullptr;

		return reinterpret_cast<utils::nt_peb*>(process_info.PebBaseAddress);
	}

	__forceinline auto get_proc_base(const HANDLE proc_handle) -> std::uintptr_t
	{
		const auto ppeb = reinterpret_cast<std::uintptr_t>(get_process_peb(proc_handle));
		const auto peb = bedaisy::read<nt_peb>(proc_handle, ppeb);
		return peb.image_base;
	}

	__forceinline auto get_module_base(const HANDLE proc_handle, const wchar_t* module_handle) -> std::uintptr_t
	{
		const auto ppeb = reinterpret_cast<std::uintptr_t>(get_process_peb(proc_handle));
		const auto peb = bedaisy::read<nt_peb>(proc_handle, ppeb);
		wchar_t full_file_name[MAX_PATH];
		std::uintptr_t module_base, file_name_ptr;

		const auto module_list_entry =
			bedaisy::read<PEB_LDR_DATA>(proc_handle, peb.ldr);

		const auto first_entry =
			reinterpret_cast<std::uintptr_t>(
				module_list_entry.InMemoryOrderModuleList.Flink);

		auto current_entry = bedaisy::read<std::uintptr_t>(proc_handle, first_entry);
		while (current_entry != first_entry)
		{
			// read full module unicode_string structure.
			file_name_ptr = bedaisy::read<ULONGLONG>(proc_handle, current_entry + 0x40);

			// read full file path.
			bedaisy::read
			(
				proc_handle,
				file_name_ptr,
				full_file_name,
				MAX_PATH
			);

			module_base = bedaisy::read<ULONGLONG>(proc_handle, current_entry + 0x20);
			if (std::wcsstr(full_file_name, module_handle))
				return module_base;

			current_entry = bedaisy::read<std::uintptr_t>(proc_handle, current_entry);
		}
		return NULL;
	}
}