// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef UEFI_HPP
#define UEFI_HPP

#include <stdint.h>

// UEFI Memory Descriptor
struct efi_memory_descriptor {
    uint32_t type;
    uint32_t padding;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
};

// UEFI Memory Types
enum efi_memory_type {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
};


struct EFI_GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};

struct EFI_CONFIGURATION_TABLE {
    EFI_GUID VendorGuid;
    uint64_t VendorTable;
};

struct EFI_TABLE_HEADER {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
};

struct EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    uint64_t FirmwareVendor;
    uint32_t FirmwareRevision;
    uint32_t Pad; 
    uint64_t ConsoleInHandle;
    uint64_t ConIn;
    uint64_t ConsoleOutHandle;
    uint64_t ConOut;
    uint64_t StandardErrorHandle;
    uint64_t StdErr;
    uint64_t RuntimeServices;
    uint64_t BootServices;
    uint64_t NumberOfTableEntries;
    uint64_t ConfigurationTable; // Pointer to array of EFI_CONFIGURATION_TABLE
};


namespace uefi
{
    /// @brief Helper: Compares two EFI GUIDs
    bool compare_guid(const EFI_GUID& a, const EFI_GUID& b);

    /// @brief Scans the UEFI configuration table for the RSDP
    /// @param mb2_info Multiboot21 info struct
    /// @return Physical address of the RSDP
    void* scan_uefi_for_rsdp(void* mb2_info);

    // More functions will be implemented when needed
} // namespace uefi



#endif // UEFI_HPP
