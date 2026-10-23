#pragma once

#if defined(_WIN64) || defined(__aarch64__) // 64-bit
    // CClass offsets
    #define SDK_IL2CPP_OFFSET_CLASS_IMAGE        0x00
    #define SDK_IL2CPP_OFFSET_CLASS_NAME         0x10
    #define SDK_IL2CPP_OFFSET_CLASS_NAMESPACE    0x18
    #define SDK_IL2CPP_OFFSET_CLASS_PARENT       0x58

    // CMethod offsets
    #define SDK_IL2CPP_OFFSET_METHOD_PTR         0x00
    #define SDK_IL2CPP_OFFSET_METHOD_VIRTUAL_PTR 0x08
    #define SDK_IL2CPP_OFFSET_METHOD_INVOKER     0x10
    #define SDK_IL2CPP_OFFSET_METHOD_NAME        0x18
    #define SDK_IL2CPP_OFFSET_METHOD_CLASS       0x20
    #define SDK_IL2CPP_OFFSET_METHOD_RETURN_TYPE 0x28
    #define SDK_IL2CPP_OFFSET_METHOD_ARGS        0x30
    #define SDK_IL2CPP_OFFSET_METHOD_ARG_COUNT   0x52

    // CField offsets
    #define SDK_IL2CPP_OFFSET_FIELD_NAME         0x00
    #define SDK_IL2CPP_OFFSET_FIELD_TYPE         0x08
    #define SDK_IL2CPP_OFFSET_FIELD_PARENT       0x10
    #define SDK_IL2CPP_OFFSET_FIELD_OFFSET       0x18

    // CImage offsets
    #define SDK_IL2CPP_OFFSET_IMAGE_NAME         0x00
    #define SDK_IL2CPP_OFFSET_IMAGE_NAME_NO_EXT  0x08
    #define SDK_IL2CPP_OFFSET_IMAGE_ASSEMBLY     0x10
    #define SDK_IL2CPP_OFFSET_IMAGE_TYPE_COUNT   0x18

    // CAssembly offsets
    #define SDK_IL2CPP_OFFSET_ASSEMBLY_IMAGE     0x00
#else // 32-bit
    // CClass offsets
    #define SDK_IL2CPP_OFFSET_CLASS_IMAGE        0x00
    #define SDK_IL2CPP_OFFSET_CLASS_NAME         0x08
    #define SDK_IL2CPP_OFFSET_CLASS_NAMESPACE    0x0C
    #define SDK_IL2CPP_OFFSET_CLASS_PARENT       0x2C

    #define SDK_IL2CPP_OFFSET_METHOD_PTR         0x00
    #define SDK_IL2CPP_OFFSET_METHOD_VIRTUAL_PTR 0x04
    #define SDK_IL2CPP_OFFSET_METHOD_INVOKER     0x08
    #define SDK_IL2CPP_OFFSET_METHOD_NAME        0x0C
    #define SDK_IL2CPP_OFFSET_METHOD_CLASS       0x10
    #define SDK_IL2CPP_OFFSET_METHOD_RETURN_TYPE 0x14
    #define SDK_IL2CPP_OFFSET_METHOD_ARGS        0x18
    #define SDK_IL2CPP_OFFSET_METHOD_ARG_COUNT   0x36

    // CField offsets
    #define SDK_IL2CPP_OFFSET_FIELD_NAME         0x00
    #define SDK_IL2CPP_OFFSET_FIELD_TYPE         0x04
    #define SDK_IL2CPP_OFFSET_FIELD_PARENT       0x08
    #define SDK_IL2CPP_OFFSET_FIELD_OFFSET       0x0C

    // CImage offsets
    #define SDK_IL2CPP_OFFSET_IMAGE_NAME         0x00
    #define SDK_IL2CPP_OFFSET_IMAGE_NAME_NO_EXT  0x04
    #define SDK_IL2CPP_OFFSET_IMAGE_ASSEMBLY     0x08
    #define SDK_IL2CPP_OFFSET_IMAGE_TYPE_COUNT   0x0C

    // CAssembly offsets
    #define SDK_IL2CPP_OFFSET_ASSEMBLY_IMAGE     0x00
#endif
