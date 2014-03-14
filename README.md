bkisofs
=======

`bkisofs` (I will often call it simply `bk`) is a simple and stable library for reading and writing `ISO9660` files. It can also read `NRG` files, but not write them. It has support for the `Joliet`, `RockRidge`, and `EL Torito` extensions. It has been tested (with positive results) on many `Linux` versions and on some `BSD` versions.

Currently `bk` is used as the backbone of ISO Master - a graphical editor for `ISO` files, but I encourage you to add `ISO` support to your application if it is appropriate. Being a library, `bk` is much easier to use from your application then `mkisofs`.
