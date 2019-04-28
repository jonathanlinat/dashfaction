#include "InjectingProcessLauncher.h"
#include <fstream>

static uintptr_t get_pe_file_entrypoint(const char* filename)
{
    std::ifstream file{filename, std::ifstream::in | std::ifstream::binary};
    if (!file)
        throw std::runtime_error(std::string("Failed to open ") + filename);
    IMAGE_DOS_HEADER dos_hdr;
    file.read(reinterpret_cast<char*>(&dos_hdr), sizeof(dos_hdr));
    if (!file || dos_hdr.e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error(std::string("Failed to read DOS header from ") + filename);

    IMAGE_NT_HEADERS32 nt_hdrs;
    file.seekg(dos_hdr.e_lfanew, std::ios_base::beg);
    file.read(reinterpret_cast<char*>(&nt_hdrs), sizeof(nt_hdrs));
    if (!file || nt_hdrs.Signature != IMAGE_NT_SIGNATURE ||
        nt_hdrs.FileHeader.SizeOfOptionalHeader < sizeof(nt_hdrs.OptionalHeader))
        throw std::runtime_error(std::string("Failed to read NT headers from ") + filename);

    return nt_hdrs.OptionalHeader.ImageBase + nt_hdrs.OptionalHeader.AddressOfEntryPoint;
}

void InjectingProcessLauncher::wait_for_process_initialization(uintptr_t entry_point, int timeout)
{
    // Change process entry point into an infinite loop (one opcode: jmp -2)
    // Based on: https://opcode0x90.wordpress.com/2011/01/15/injecting-dll-into-process-on-load/
    char buf[2];
    void* entry_point_ptr = reinterpret_cast<void*>(entry_point);
    ProcessMemoryProtection protect{m_process, entry_point_ptr, 2, PAGE_EXECUTE_READWRITE};
    m_process.read_mem(buf, entry_point_ptr, 2);
    m_process.write_mem(entry_point_ptr, "\xEB\xFE", 2);
    FlushInstructionCache(m_process.get_handle(), entry_point_ptr, 2);
    // Resume main thread
    m_thread.resume();
    // Wait untill main thread reaches the entry point
    CONTEXT context;
    for (int i = 0; i < timeout / 50; ++i) {
        Sleep(50);
        context.ContextFlags = CONTEXT_CONTROL;
        try {
            m_thread.get_context(&context);
        }
        catch (const Win32Error&) {
            if (m_thread.get_exit_code() != STILL_ACTIVE)
                throw ProcessTerminatedError();
            throw;
        }

        if (context.Eip == entry_point)
            break;
    }
    if (context.Eip != entry_point)
        THROW_EXCEPTION("timeout");
    // Suspend main thread
    m_thread.suspend();
    // Revert changes to entry point
    m_process.write_mem(entry_point_ptr, buf, 2);
    FlushInstructionCache(m_process.get_handle(), entry_point_ptr, 2);
}

InjectingProcessLauncher::InjectingProcessLauncher(
    const char* app_name, const char* work_dir, const char* command_line, STARTUPINFO& startup_info, int timeout)
{
    PROCESS_INFORMATION process_info;
    BOOL result = CreateProcessA(app_name, const_cast<char*>(command_line), nullptr, nullptr, FALSE, CREATE_SUSPENDED,
                                 nullptr, work_dir, &startup_info, &process_info);
    if (!result) {
        THROW_WIN32_ERROR();
    }

    m_process = Process{process_info.hProcess};
    m_thread = Thread{process_info.hThread};

    uintptr_t entry_point = get_pe_file_entrypoint(app_name);
    wait_for_process_initialization(entry_point, timeout);
}