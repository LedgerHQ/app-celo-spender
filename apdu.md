        ## ISO/IEC 7816-4 (2005) APDU errors (SW1/SW2)

        | Error Code | Error Description                            |
        |------------|---------------------------------------------|
        | 0x6700     | wrong length (general error)                 |
        | 0x6900     | command not allowed (general error)          |
        | 0x6981     | command incompatible with file structure     |
        | 0x6982     | security status not satisfied                |
        | 0x6A00     | wrong parameters p1/p2 (general error)       |
        | 0x6A80     | incorrect parameters in command data field   |
        | 0x6A81     | function not supported                       |
        | 0x6A82     | file or application not found                |
        | 0x6A83     | record not found                             |
        | 0x6A84     | not enough memory space in the file          |
        | 0x6A85     | command length inconsistent with TLV structure |
        | 0x6A86     | incorrect parameters p1/p2                   |
        | 0x6A87     | command length inconsistent with p1/p2       |
        | 0x6A88     | referenced data or reference data not found  |
        | 0x6A89     | file already exists                          |
        | 0x6A8A     | file name already exists                     |
        |-------------------Personalized status word ---------------|
        | 0x6A8B     | error while hashing                          |
        | 0x6A8C     | error while deriving path                    |
