var acpi_8c =
[
    [ "btable_t", "acpi_8c.html#structbtable__t", [
      [ "root", "acpi_8c.html#a8cd427b31aa6a4809384637aa3c4e209", null ],
      [ "next_page", "acpi_8c.html#a7e8f21fb95997de5ef00a3ba2ae043ee", null ],
      [ "term_page", "acpi_8c.html#a7dd37aca0c98bf83bc4e6ee7e4348540", null ]
    ] ],
    [ "acpi_rsdp", "acpi_8c.html#structacpi__rsdp", [
      [ "signature", "acpi_8c.html#a1e6a2cb55b9aacec319f2fbb17fa19ec", null ],
      [ "checksum", "acpi_8c.html#a403fb69dfab8f5c232ef69e7d8f7641f", null ],
      [ "oemid", "acpi_8c.html#a46ccccfd346b6afa44446208a6c22414", null ],
      [ "revision", "acpi_8c.html#a9e0f10f5787c7927e378a05152dabd47", null ],
      [ "ptr_rsdt", "acpi_8c.html#a71bee601b91d05bdffaea8e3b8a43dc1", null ],
      [ "length", "acpi_8c.html#a8fab6d14aecb51b0a3e69e6987df79db", null ],
      [ "ptr_xsdt", "acpi_8c.html#a53eefb1ebff48cf7a7c8a2dc65ab2e1a", null ],
      [ "checksum_ex", "acpi_8c.html#a52cbcc39acb2b0894e67eeacc8bea7ff", null ],
      [ "reserved", "acpi_8c.html#af6563040515010555054d4f3236dc5fe", null ]
    ] ],
    [ "acpi_rsdt", "acpi_8c.html#structacpi__rsdt", [
      [ "hdr", "acpi_8c.html#a05a5549a41deadd2f3f9b9da7000c789", null ],
      [ "ptr_table", "acpi_8c.html#a3c8be54505bc1e034c15bd4b8eb59405", null ]
    ] ],
    [ "acpi_xsdt", "acpi_8c.html#structacpi__xsdt", [
      [ "hdr", "acpi_8c.html#a3b0ec4dcbb7695c1d17acc51a197113a", null ],
      [ "ptr_table", "acpi_8c.html#af8352d2321fdbecbe9f6b6fe648a82f2", null ]
    ] ],
    [ "acpi", "acpi_8c.html#structacpi", [
      [ "version", "acpi_8c.html#a17c90ad3571f39474c0974fa4dec98c5", null ],
      [ "rsdp", "acpi_8c.html#a91969859649edf7f1ed6ad3bbdee830d", null ],
      [ "rsdt", "acpi_8c.html#a14d3a269d482cb5d7f28510aaf5c655c", null ],
      [ "xsdt", "acpi_8c.html#a09c0af1dd9c9e3c4db022a578d760fcc", null ],
      [ "fadt", "acpi_8c.html#a4221764eacca472cfa539e3aaf4e0fad", null ],
      [ "madt", "acpi_8c.html#ae40b5966cbd959e2e0b55a5c76ff96dd", null ],
      [ "mcfg", "acpi_8c.html#abcd7a0fcbdf15722f3b368fb8897de83", null ]
    ] ],
    [ "SIGNATURE_RSDP", "acpi_8c.html#a21668ae271765e086047838ff9325d25", null ],
    [ "SIGNATURE_MADT", "acpi_8c.html#a748d6f1fd73e8a47dab09e2fa409d135", null ],
    [ "SIGNATURE_BOOT", "acpi_8c.html#afcb7d82f62ab6d44c027a9c1586334e1", null ],
    [ "SIGNATURE_FADT", "acpi_8c.html#ab5a9a788ec227d1165c4547a61d335d1", null ],
    [ "SIGNATURE_HPET", "acpi_8c.html#a178b9668adc5f32b47c8b77c5cf62228", null ],
    [ "SIGNATURE_MCFG", "acpi_8c.html#ae005b07fad1f00e5d1e2550d6d4875c4", null ],
    [ "SIGNATURE_SRAT", "acpi_8c.html#a967ac92d39e98668e9e6cc4df1cbf546", null ],
    [ "SIGNATURE_SSDT", "acpi_8c.html#aff810f33abad48d8a25f4927042700e8", null ],
    [ "SIGNATURE_WAET", "acpi_8c.html#ac02afa5b1cf0d2c5d6f671769009135a", null ],
    [ "PAGE_ALIGN_DOWN", "acpi_8c.html#a9ce15055d80356b2621b9aef7ad38b3b", null ],
    [ "PAGE_ALIGN_UP", "acpi_8c.html#adfa9e0bbc246ce204000d4bc6cc66270", null ],
    [ "read_fadt", "acpi_8c.html#ae3cbea7d2c62a4f02fd1b0f90e508636", null ],
    [ "read_madt", "acpi_8c.html#ac4e48bb79e81d20d71010b0c505da7e0", null ],
    [ "read_mcfg", "acpi_8c.html#a3c502236ce0f734b2042b89b369ab558", null ],
    [ "read_table", "acpi_8c.html#a9c27e3eb6aeb4eebab9f3185e355ba11", null ],
    [ "is_mapped", "acpi_8c.html#ad004d08f76a5fce2edf15318b22fa512", null ],
    [ "alloc_page", "acpi_8c.html#ab5cf7227f3f9fa2bed8390503d308d4a", null ],
    [ "create_page", "acpi_8c.html#a5554a059d88308b49d58baea03c7ec12", null ],
    [ "map_range", "acpi_8c.html#a2c16b4c762786010a35ece3f5e8ae50e", null ],
    [ "map_table", "acpi_8c.html#a5a4909bc93e93b592b4c181852778d59", null ],
    [ "read_xsdt", "acpi_8c.html#aa63a211de6e1b5dd1db9fb6cd979f492", null ],
    [ "read_rsdt", "acpi_8c.html#a301784ee476db85422cdc659e7bd4d05", null ],
    [ "find_rsdp", "acpi_8c.html#a38d880d7e8f705ea618092e3c2b8ae2f", null ],
    [ "acpi_init", "acpi_8c.html#ac89f7fd8333fcd61dea9163a99c0fd6c", null ],
    [ "acpi_version", "acpi_8c.html#a4f28496746c3be5d94bfaa75c75449ad", null ],
    [ "acpi_fadt", "acpi_8c.html#af72c41c9c4ea1155d547e5e140388d5e", null ],
    [ "acpi_madt", "acpi_8c.html#a48758ba9dd14a15e7f2b9d3d6e3de355", null ],
    [ "madt_find", "acpi_8c.html#ab867118bca3fcf0a50cab4dd680725ba", null ],
    [ "acpi_next_local_apic", "acpi_8c.html#a35e731d48fbd611a1e800f109fd85820", null ],
    [ "acpi_next_io_apic", "acpi_8c.html#adf13a1b723781b7bcdd181fb0545e5c5", null ],
    [ "acpi_next_iso", "acpi_8c.html#af7948f7e3d1fa0040b2173ec5a15ff63", null ],
    [ "acpi_next_mcfg_addr", "acpi_8c.html#a89cb61180510f686a95ebc237695a464", null ],
    [ "PACKSTRUCT", "acpi_8c.html#aaf0e917a27dad33235a10d31eaabf8b6", null ],
    [ "acpi", "acpi_8c.html#a73341b6c2107a794ea4d84ce4b785b6e", null ]
];