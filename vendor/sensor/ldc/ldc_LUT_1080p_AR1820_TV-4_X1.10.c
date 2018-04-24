#include "mmpf_typedef.h"
#include "mmpf_ldc.h"

MMP_USHORT m_usPositionX_FHD[LDC_X_POS_ARRAY_SIZE] = 
{
			0x0000, 
			0x002f, 
			0x005e, 
			0x008c, 
			0x00bb, 
			0x00ea, 
			0x0119, 
			0x0148, 
			0x0177, 
			0x01a5, 
			0x01d4, 
			0x0203, 
			0x0232, 
			0x0261, 
			0x0290, 
			0x02be, 
			0x02ed, 
			0x031c, 
			0x034b, 
			0x037a, 
			0x03a9, 
			0x03d7, 
			0x0406, 
			0x0435, 
			0x0464, 
			0x0493, 
			0x04c2, 
			0x04f0, 
			0x051f, 
			0x054e, 
			0x057d, 
			0x05ac, 
			0x05db, 
			0x0609, 
			0x0638, 
			0x0667, 
			0x0696, 
			0x06c5, 
			0x06f4, 
			0x0722, 
			0x0751, 
			0x0780, 
};

MMP_USHORT m_usPositionY_FHD[LDC_Y_POS_ARRAY_SIZE] = 
{
			0x0000, 
			0x0023, 
			0x0046, 
			0x0069, 
			0x008b, 
			0x00ae, 
			0x00d1, 
			0x00f4, 
			0x0117, 
			0x013a, 
			0x015c, 
			0x017f, 
			0x01a2, 
			0x01c5, 
			0x01e8, 
			0x020b, 
			0x022d, 
			0x0250, 
			0x0273, 
			0x0296, 
			0x02b9, 
			0x02dc, 
			0x02fe, 
			0x0321, 
			0x0344, 
			0x0367, 
			0x038a, 
			0x03ad, 
			0x03cf, 
			0x03f2, 
			0x0415, 
			0x0438, 
};

MMP_ULONG m_ulDeltaMemA_000_127_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x062d03c4,
		0x04ba033e,
		0x037d02bd,
		0x026f023e,
		0x019501c5,
		0x00f00158,
		0x007a00f7,
		0x002f00a5,
		0x00080066,
		0xfffb003d,
		0xfffe002a,
		0x00050030,
		0x0001004e,
		0xffe80084,
		0xffb100cb,
		0xff510126,
		0xfec3018d,
		0xfe040200,
		0xfd10027d,
		0xfbe902fe,
		0xfa930381,
		0x05cf0319,
		0x045d02a1,
		0x0322022b,
		0x021801b8,
		0x01440149,
		0x00a600e5,
		0x0039008b,
		0xfffb0040,
		0xffe20006,
		0xffe5ffdf,
		0xfff9ffce,
		0x0011ffd3,
		0x001ffff0,
		0x00150021,
		0xffeb0063,
		0xff9600b6,
		0xff110117,
		0xfe590180,
		0xfd6901f1,
		0xfc450266,
		0xfaf102dd,
		0x057c0282,
		0x040b0218,
		0x02d201b0,
		0x01cb014a,
		0x00fa00e7,
		0x0063008d,
		0x0000003d,
		0xffccfff9,
		0xffc0ffc5,
		0xffd2ffa2,
		0xfff6ff92,
		0x001dff97,
		0x003affb1,
		0x003effde,
		0x00200019,
		0xffd50064,
		0xff5800ba,
		0xfea40118,
		0xfdb8017c,
		0xfc9601e4,
		0xfb43024d,
		0x053301f9,
		0x03c2019f,
		0x028a0146,
		0x018500ee,
		0x00ba009b,
		0x0028004e,
		0xffcc0008,
		0xffa2ffcd,
		0xffa2ff9f,
		0xffc1ff82,
		0xfff2ff74,
		0x0028ff78,
		0x0052ff8e,
		0x0062ffb5,
		0x004fffe9,
		0x000c002a,
		0xff960073,
		0xfee800c4,
		0xfdff011a,
		0xfcdf0173,
		0xfb8c01cc,
		0x04f7017f,
		0x03860135,
		0x024f00ed,
		0x014c00a6,
		0x00830062,
		0xfff50023,
		0xffa1ffea,
		0xff7effba,
		0xff88ff94,
		0xffb1ff7b,
		0xffefff70,
		0x0031ff73,
		0x0067ff86,
		0x0081ffa6,
		0x0076ffd1,
		0x003c0005,
		0xffcb0043,
		0xff200084,
		0xfe3a00c9,
		0xfd1b0112,
		0xfbc8015a,
		0x04c90110,
		0x035800d9,
		0x022100a3,
		0x011f006e,
		0x0058003a,
		0xffcf000a,
		0xff7effdf,
		0xff63ffbb,
		0xff73ff9f,
		0xffa7ff8b,
		0xffeeff83,
		0x0037ff86,
		0x0077ff94,
		0x009affac,
		0x0096ffcc,
		0x0060fff4,
		0xfff30022,
		0xff4b0054,
		0xfe670088,
		0xfd4900be,
		0xfbf700f5,
		0x04a700a9,
		0x03360086,

};

MMP_ULONG m_ulDeltaMemA_128_255_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x02000063,
		0x00ff0040,
		0x003a001e,
		0xffb30000,
		0xff66ffe3,
		0xff4fffcc,
		0xff65ffba,
		0xff9dffad,
		0xffecffa7,
		0x003dffaa,
		0x0082ffb3,
		0x00abffc2,
		0x00acffd7,
		0x007afff1,
		0x0011000f,
		0xff6b002f,
		0xfe880051,
		0xfd6a0074,
		0xfc180097,
		0x04950047,
		0x03240038,
		0x01ed0029,
		0x00ed0019,
		0x002a000c,
		0xffa3fffe,
		0xff59fff2,
		0xff44ffe8,
		0xff5cffe0,
		0xff99ffdb,
		0xffebffd8,
		0x0040ffd9,
		0x0089ffdd,
		0x00b5ffe4,
		0x00b8ffed,
		0x0089fff8,
		0x00210005,
		0xff7d0012,
		0xfe9a0021,
		0xfd7d0030,
		0xfc2b0040,
		0x0491ffe9,
		0x0320ffee,
		0x01eafff2,
		0x00eafff8,
		0x0026fffc,
		0xffa00001,
		0xff560005,
		0xff410008,
		0xff5b000a,
		0xff98000c,
		0xffea000d,
		0x0041000d,
		0x008a000b,
		0x00b70009,
		0x00bb0006,
		0x008c0003,
		0x0025fffe,
		0xff80fffa,
		0xfe9efff5,
		0xfd80fff0,
		0xfc2fffeb,
		0x049cff89,
		0x032bffa2,
		0x01f5ffbb,
		0x00f4ffd4,
		0x0030ffec,
		0xffa90001,
		0xff5e0016,
		0xff480027,
		0xff600034,
		0xff9b003e,
		0xffeb0041,
		0x003f0040,
		0x0087003a,
		0x00b1002e,
		0x00b4001f,
		0x0083000c,
		0x001bfff7,
		0xff76ffe0,
		0xfe93ffc8,
		0xfd75ffae,
		0xfc24ff95,
		0x04b6ff24,
		0x0345ff51,
		0x020fff7e,
		0x010dffab,
		0x0048ffd5,
		0xffbffffc,
		0xff710020,
		0xff57003e,
		0xff6b0055,
		0xffa20065,
		0xffed006d,
		0x003b006a,
		0x007d005f,
		0x00a3004a,
		0x00a20030,
		0x006f000e,
		0x0004ffe9,
		0xff5dffc0,
		0xfe79ff95,
		0xfd5bff68,
		0xfc09ff3b,
		0x04defebb,
		0x036dfefa,
		0x0236ff39,
		0x0134ff78,
		0x006cffb4,
		0xffe0ffec,
		0xff8f001e,
		0xff700048,
		0xff7d0069,
		0xffac007f,
		0xffef0089,
		0x00350086,
		0x006f0075,
		0x008e0059,
		0x00870034,
		0x004f0005,
		0xffe1ffd0,
		0xff37ff96,
		0xfe52ff59,
		0xfd34ff1a,
		0xfbe2fedb,
		0x0514fe46,
		0x03a3fe97,
		0x026bfee8,
		0x0167ff37,

};

MMP_ULONG m_ulDeltaMemA_256_335_FHD[LDC_DELTA_ARRAY_SIZE] =
{
        0x009dff84,
		0x000dffca,
		0xffb50009,
		0xff900040,
		0xff940069,
		0xffb90085,
		0xfff10091,
		0x002d008d,
		0x005d0079,
		0x00730055,
		0x00630026,
		0x0026ffeb,
		0xffb2ffa8,
		0xff06ff5e,
		0xfe1eff10,
		0xfcfefebf,
		0xfbacfe6e,
		0x0556fdc4,
		0x03e5fe27,
		0x02adfe87,
		0x01a7fee6,
		0x00d8ff41,
		0x0045ff95,
		0xffe5ffe1,
		0xffb70020,
		0xffb00051,
		0xffc90072,
		0xfff40081,
		0x0023007c,
		0x00470063,
		0x0051003a,
		0x00380002,
		0xfff2ffbc,
		0xff78ff6c,
		0xfec7ff15,
		0xfdddfeb7,
		0xfcbcfe57,
		0xfb69fdf5,
		0x05a4fd35,
		0x0432fda7,
		0x02f8fe16,
		0x01effe83,
		0x011dfeeb,
		0x0083ff4a,
		0x001cffa0,
		0xffe3ffe8,
		0xffd1001f,
		0xffdb0044,
		0xfff80054,
		0x0018004f,
		0x002d0034,
		0x002a0005,
		0x0006ffc6,
		0xffb7ff76,
		0xff36ff1b,
		0xfe81feb8,
		0xfd93fe4d,
		0xfc6ffdde,
		0xfb1cfd6e,
		0x05fdfc94,
		0x048afd13,
		0x034ffd8f,
		0x0243fe08,
		0x016bfe7b,
		0x00cafee5,
		0x0059ff43,
		0x0014ff92,
		0xfff4ffce,
		0xfff0fff6,
		0xfffc0008,
		0x000b0002,
		0x0011ffe5,
		0xffffffb2,
		0xffcfff6d,
		0xff75ff16,
		0xfeebfeb1,
		0xfe2ffe43,
		0xfd3efdcc,
		0xfc18fd50,
		0xfac3fcd3,
};

MMP_ULONG m_ulDeltaMemB_000_127_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x056d0381,
		0x041702fe,
		0x02f0027d,
		0x01fc0200,
		0x013d018d,
		0x00af0126,
		0x004f00cb,
		0x00180084,
		0xffff004e,
		0xfffb0030,
		0x0002002a,
		0x0005003d,
		0xfff80066,
		0xffd100a5,
		0xff8600f7,
		0xff100158,
		0xfe6b01c5,
		0xfd91023e,
		0xfc8302bd,
		0xfb46033e,
		0xf9d703c2,
		0x050f02dd,
		0x03bb0266,
		0x029701f1,
		0x01a70180,
		0x00ef0117,
		0x006a00b6,
		0x00150063,
		0xffeb0021,
		0xffe1fff0,
		0xffefffd3,
		0x0007ffce,
		0x001bffdf,
		0x001e0006,
		0x00050040,
		0xffc7008b,
		0xff5a00e5,
		0xfebc0149,
		0xfde801b8,
		0xfcde022b,
		0xfba302a1,
		0xfa360318,
		0x04bd024d,
		0x036a01e4,
		0x0248017c,
		0x015c0118,
		0x00a800ba,
		0x002b0064,
		0xffe00019,
		0xffc2ffde,
		0xffc6ffb1,
		0xffe3ff97,
		0x000aff92,
		0x002effa2,
		0x0040ffc5,
		0x0034fff9,
		0x0000003d,
		0xff9d008d,
		0xff0600e7,
		0xfe35014a,
		0xfd2e01b0,
		0xfbf50218,
		0xfa890281,
		0x047401cc,
		0x03210173,
		0x0201011a,
		0x011800c4,
		0x006a0073,
		0xfff4002a,
		0xffb1ffe9,
		0xff9effb5,
		0xffaeff8e,
		0xffd8ff78,
		0x000eff74,
		0x003fff82,
		0x005eff9f,
		0x005effcd,
		0x00340008,
		0xffd8004e,
		0xff46009b,
		0xfe7b00ee,
		0xfd760146,
		0xfc3e019f,
		0xfad101f8,
		0x0438015a,
		0x02e50112,
		0x01c600c9,
		0x00e00084,
		0x00350043,
		0xffc40005,
		0xff8affd1,
		0xff7fffa6,
		0xff99ff86,
		0xffcfff73,
		0x0011ff70,
		0x004fff7b,
		0x0078ff94,
		0x0082ffba,
		0x005fffea,
		0x000b0023,
		0xff7d0062,
		0xfeb400a6,
		0xfdb100ed,
		0xfc7a0135,
		0xfb0e017e,
		0x040900f5,
		0x02b700be,
		0x01990088,
		0x00b50054,
		0x000d0022,
		0xffa0fff4,
		0xff6affcc,
		0xff66ffac,
		0xff89ff94,
		0xffc9ff86,
		0x0012ff83,
		0x0059ff8b,
		0x008dff9f,
		0x009dffbb,
		0x0082ffdf,
		0x0031000a,
		0xffa8003a,
		0xfee1006e,
		0xfddf00a3,
		0xfca800d9,
		0xfb3c0110,
		0x03e80097,
		0x02960074,
};

MMP_ULONG m_ulDeltaMemB_128_255_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x01780051,
		0x0095002f,
		0xffef000f,
		0xff86fff1,
		0xff54ffd7,
		0xff55ffc2,
		0xff7effb3,
		0xffc3ffaa,
		0x0014ffa7,
		0x0063ffad,
		0x009bffba,
		0x00b1ffcc,
		0x009affe3,
		0x004d0000,
		0xffc6001e,
		0xff010040,
		0xfe000063,
		0xfcca0086,
		0xfb5e00a8,
		0x03d50040,
		0x02830030,
		0x01660021,
		0x00830012,
		0xffdf0005,
		0xff77fff8,
		0xff48ffed,
		0xff4bffe4,
		0xff77ffdd,
		0xffc0ffd9,
		0x0015ffd8,
		0x0067ffdb,
		0x00a4ffe0,
		0x00bcffe8,
		0x00a7fff2,
		0x005dfffe,
		0xffd6000c,
		0xff130019,
		0xfe130029,
		0xfcdc0038,
		0xfb700047,
		0x03d1ffeb,
		0x0280fff0,
		0x0162fff5,
		0x0080fffa,
		0xffdbfffe,
		0xff740003,
		0xff450006,
		0xff490009,
		0xff76000b,
		0xffbf000d,
		0x0016000d,
		0x0068000c,
		0x00a5000a,
		0x00bf0008,
		0x00aa0005,
		0x00600001,
		0xffdafffc,
		0xff16fff8,
		0xfe16fff2,
		0xfce0ffee,
		0xfb74ffe9,
		0x03dcff95,
		0x028bffae,
		0x016dffc8,
		0x008affe0,
		0xffe5fff7,
		0xff7d000c,
		0xff4c001f,
		0xff4f002e,
		0xff79003a,
		0xffc10040,
		0x00150041,
		0x0065003e,
		0x00a00034,
		0x00b80027,
		0x00a20016,
		0x00570001,
		0xffd0ffec,
		0xff0cffd4,
		0xfe0bffbb,
		0xfcd5ffa2,
		0xfb69ff89,
		0x03f7ff3b,
		0x02a5ff68,
		0x0187ff95,
		0x00a3ffc0,
		0xfffcffe9,
		0xff91000e,
		0xff5e0030,
		0xff5d004a,
		0xff83005f,
		0xffc5006a,
		0x0013006d,
		0x005e0065,
		0x00950055,
		0x00a9003e,
		0x008f0020,
		0x0041fffc,
		0xffb8ffd5,
		0xfef3ffab,
		0xfdf1ff7e,
		0xfcbbff51,
		0xfb4fff25,
		0x041efedb,
		0x02ccff1a,
		0x01aeff59,
		0x00c9ff96,
		0x001fffd0,
		0xffb10005,
		0xff790034,
		0xff720059,
		0xff910075,
		0xffcb0086,
		0x00110089,
		0x0054007f,
		0x00830069,
		0x00900048,
		0x0071001e,
		0x0020ffec,
		0xff94ffb4,
		0xfeccff78,
		0xfdcaff39,
		0xfc93fefa,
		0xfb27febc,
		0x0454fe6e,
		0x0302febf,
		0x01e2ff10,
		0x00faff5e,
};

MMP_ULONG m_ulDeltaMemB_256_335_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x004effa8,
		0xffdaffeb,
		0xff9d0026,
		0xff8d0055,
		0xffa30079,
		0xffd3008d,
		0x000f0091,
		0x00470085,
		0x006c0069,
		0x00700040,
		0x004b0009,
		0xfff3ffca,
		0xff63ff84,
		0xfe99ff37,
		0xfd95fee8,
		0xfc5dfe97,
		0xfaf1fe46,
		0x0497fdf5,
		0x0344fe57,
		0x0223feb7,
		0x0139ff15,
		0x0088ff6c,
		0x000effbc,
		0xffc80002,
		0xffaf003a,
		0xffb90063,
		0xffdd007c,
		0x000c0081,
		0x00370072,
		0x00500051,
		0x00490020,
		0x001bffe1,
		0xffbbff95,
		0xff28ff41,
		0xfe59fee6,
		0xfd53fe87,
		0xfc1bfe27,
		0xfaaefdc5,
		0x04e4fd6e,
		0x0391fdde,
		0x026dfe4d,
		0x017ffeb8,
		0x00caff1b,
		0x0049ff76,
		0xfffaffc6,
		0xffd60005,
		0xffd30034,
		0xffe8004f,
		0x00080054,
		0x00250044,
		0x002f001f,
		0x001dffe8,
		0xffe4ffa0,
		0xff7dff4a,
		0xfee3feeb,
		0xfe11fe83,
		0xfd08fe16,
		0xfbcefda7,
		0xfa61fd36,
		0x053dfcd3,
		0x03e8fd50,
		0x02c2fdcc,
		0x01d1fe43,
		0x0115feb1,
		0x008bff16,
		0x0031ff6d,
		0x0001ffb2,
		0xffefffe5,
		0xfff50002,
		0x00040008,
		0x0010fff6,
		0x000cffce,
		0xffecff92,
		0xffa7ff43,
		0xff36fee5,
		0xfe95fe7b,
		0xfdbdfe08,
		0xfcb1fd8f,
		0xfb76fd13,
		0xfa08fc95,
		
};

MMP_ULONG m_ulDeltaMemC_000_127_FHD[LDC_DELTA_ARRAY_SIZE] =
{
        0x05fd036c,
		0x048a02ed,
		0x034f0271,
		0x024301f8,
		0x016b0185,
		0x00ca011b,
		0x005900bd,
		0x0014006e,
		0xfff40032,
		0xfff0000a,
		0xfffcfff8,
		0x000bfffe,
		0x0011001b,
		0xffff004e,
		0xffcf0093,
		0xff7500ea,
		0xfeeb014f,
		0xfe2f01bd,
		0xfd3e0234,
		0xfc1802b0,
		0xfac3032d,
		0x05a402cb,
		0x04320259,
		0x02f801ea,
		0x01ef017d,
		0x011d0115,
		0x008300b6,
		0x001c0060,
		0xffe30018,
		0xffd1ffe1,
		0xffdbffbc,
		0xfff8ffac,
		0x0018ffb1,
		0x002dffcc,
		0x002afffb,
		0x0006003a,
		0xffb7008a,
		0xff3600e5,
		0xfe810148,
		0xfd9301b3,
		0xfc6f0222,
		0xfb1c0292,
		0x0556023c,
		0x03e501d9,
		0x02ad0179,
		0x01a7011a,
		0x00d800bf,
		0x0045006b,
		0xffe5001f,
		0xffb7ffe0,
		0xffb0ffaf,
		0xffc9ff8e,
		0xfff4ff7f,
		0x0023ff84,
		0x0047ff9d,
		0x0051ffc6,
		0x0038fffe,
		0xfff20044,
		0xff780094,
		0xfec700eb,
		0xfddd0149,
		0xfcbc01a9,
		0xfb69020b,
		0x051401ba,
		0x03a30169,
		0x026b0118,
		0x016700c9,
		0x009d007c,
		0x000d0036,
		0xffb5fff7,
		0xff90ffc0,
		0xff94ff97,
		0xffb9ff7b,
		0xfff1ff6f,
		0x002dff73,
		0x005dff87,
		0x0073ffab,
		0x0063ffda,
		0x00260015,
		0xffb20058,
		0xff0600a2,
		0xfe1e00f0,
		0xfcfe0141,
		0xfbac0192,
		0x04de0145,
		0x036d0106,
		0x023600c7,
		0x01340088,
		0x006c004c,
		0xffe00014,
		0xff8fffe2,
		0xff70ffb8,
		0xff7dff97,
		0xffacff81,
		0xffefff77,
		0x0035ff7a,
		0x006fff8b,
		0x008effa7,
		0x0087ffcc,
		0x004ffffb,
		0xffe10030,
		0xff37006a,
		0xfe5200a7,
		0xfd3400e6,
		0xfbe20125,
		0x04b600dc,
		0x034500af,
		0x020f0082,
		0x010d0055,
		0x0048002b,
		0xffbf0004,
		0xff71ffe0,
		0xff57ffc2,
		0xff6bffab,
		0xffa2ff9b,
		0xffedff93,
		0x003bff96,
		0x007dffa1,
		0x00a3ffb6,
		0x00a2ffd0,
		0x006ffff2,
		0x00040017,
		0xff5d0040,
		0xfe79006b,
		0xfd5b0098,
		0xfc0900c5,
		0x049c0077,
		0x032b005e,
};

MMP_ULONG m_ulDeltaMemC_128_255_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x01f50045,
		0x00f4002c,
		0x00300014,
		0xffa9ffff,
		0xff5effea,
		0xff48ffd9,
		0xff60ffcc,
		0xff9bffc2,
		0xffebffbf,
		0x003fffc0,
		0x0087ffc6,
		0x00b1ffd2,
		0x00b4ffe1,
		0x0083fff4,
		0x001b0009,
		0xff760020,
		0xfe930038,
		0xfd750052,
		0xfc24006b,
		0x04910017,
		0x03200012,
		0x01ea000e,
		0x00ea0008,
		0x00260004,
		0xffa0ffff,
		0xff56fffb,
		0xff41fff8,
		0xff5bfff6,
		0xff98fff4,
		0xffeafff3,
		0x0041fff3,
		0x008afff5,
		0x00b7fff7,
		0x00bbfffa,
		0x008cfffd,
		0x00250002,
		0xff800006,
		0xfe9e000b,
		0xfd800010,
		0xfc2f0015,
		0x0495ffb9,
		0x0324ffc8,
		0x01edffd7,
		0x00edffe7,
		0x002afff4,
		0xffa30002,
		0xff59000e,
		0xff440018,
		0xff5c0020,
		0xff990025,
		0xffeb0028,
		0x00400027,
		0x00890023,
		0x00b5001c,
		0x00b80013,
		0x00890008,
		0x0021fffb,
		0xff7dffee,
		0xfe9affdf,
		0xfd7dffd0,
		0xfc2bffc0,
		0x04a7ff57,
		0x0336ff7a,
		0x0200ff9d,
		0x00ffffc0,
		0x003affe2,
		0xffb30000,
		0xff66001d,
		0xff4f0034,
		0xff650046,
		0xff9d0053,
		0xffec0059,
		0x003d0056,
		0x0082004d,
		0x00ab003e,
		0x00ac0029,
		0x007a000f,
		0x0011fff1,
		0xff6bffd1,
		0xfe88ffaf,
		0xfd6aff8c,
		0xfc18ff69,
		0x04c9fef0,
		0x0358ff27,
		0x0221ff5d,
		0x011fff92,
		0x0058ffc6,
		0xffcffff6,
		0xff7e0021,
		0xff630045,
		0xff730061,
		0xffa70075,
		0xffee007d,
		0x0037007a,
		0x0077006c,
		0x009a0054,
		0x00960034,
		0x0060000c,
		0xfff3ffde,
		0xff4bffac,
		0xfe67ff78,
		0xfd49ff42,
		0xfbf7ff0b,
		0x04f7fe81,
		0x0386fecb,
		0x024fff13,
		0x014cff5a,
		0x0083ff9e,
		0xfff5ffdd,
		0xffa10016,
		0xff7e0046,
		0xff88006c,
		0xffb10085,
		0xffef0090,
		0x0031008d,
		0x0067007a,
		0x0081005a,
		0x0076002f,
		0x003cfffb,
		0xffcbffbd,
		0xff20ff7c,
		0xfe3aff37,
		0xfd1bfeee,
		0xfbc8fea6,
		0x0533fe07,
		0x03c2fe61,
		0x028afeba,
		0x0185ff12,
};

MMP_ULONG m_ulDeltaMemC_256_335_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x00baff65,
		0x0028ffb2,
		0xffccfff8,
		0xffa20033,
		0xffa20061,
		0xffc1007e,
		0xfff2008c,
		0x00280088,
		0x00520072,
		0x0062004b,
		0x004f0017,
		0x000cffd6,
		0xff96ff8d,
		0xfee8ff3c,
		0xfdfffee6,
		0xfcdffe8d,
		0xfb8cfe34,
		0x057cfd7e,
		0x040bfde8,
		0x02d2fe50,
		0x01cbfeb6,
		0x00faff19,
		0x0063ff73,
		0x0000ffc3,
		0xffcc0007,
		0xffc0003b,
		0xffd2005e,
		0xfff6006e,
		0x001d0069,
		0x003a004f,
		0x003e0022,
		0x0020ffe7,
		0xffd5ff9c,
		0xff58ff46,
		0xfea4fee8,
		0xfdb8fe84,
		0xfc96fe1c,
		0xfb43fdb3,
		0x05cffce7,
		0x045dfd5f,
		0x0322fdd5,
		0x0218fe48,
		0x0144feb7,
		0x00a6ff1b,
		0x0039ff75,
		0xfffbffc0,
		0xffe2fffa,
		0xffe50021,
		0xfff90032,
		0x0011002d,
		0x001f0010,
		0x0015ffdf,
		0xffebff9d,
		0xff96ff4a,
		0xff11fee9,
		0xfe59fe80,
		0xfd69fe0f,
		0xfc45fd9a,
		0xfaf1fd23,
		0x062cfc3e,
		0x04b9fcc4,
		0x037cfd45,
		0x026efdc4,
		0x0194fe3c,
		0x00eefeaa,
		0x0079ff0b,
		0x002eff5d,
		0x0008ff9b,
		0xfffbffc5,
		0xfffeffd8,
		0x0005ffd2,
		0x0001ffb3,
		0xffe9ff7e,
		0xffb2ff36,
		0xff52fedc,
		0xfec4fe74,
		0xfe06fe02,
		0xfd11fd85,
		0xfbeafd04,
		0xfa94fc81,
};

MMP_ULONG m_ulDeltaMemD_000_127_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x053d032d,
		0x03e802b0,
		0x02c20234,
		0x01d101bd,
		0x0115014f,
		0x008b00ea,
		0x00310093,
		0x0001004e,
		0xffef001b,
		0xfff5fffe,
		0x0004fff8,
		0x0010000a,
		0x000c0032,
		0xffec006e,
		0xffa700bd,
		0xff36011b,
		0xfe950185,
		0xfdbd01f8,
		0xfcb10271,
		0xfb7602ed,
		0xfa08036b,
		0x04e40292,
		0x03910222,
		0x026d01b3,
		0x017f0148,
		0x00ca00e5,
		0x0049008a,
		0xfffa003a,
		0xffd6fffb,
		0xffd3ffcc,
		0xffe8ffb1,
		0x0008ffac,
		0x0025ffbc,
		0x002fffe1,
		0x001d0018,
		0xffe40060,
		0xff7d00b6,
		0xfee30115,
		0xfe11017d,
		0xfd0801ea,
		0xfbce0259,
		0xfa6102ca,
		0x0497020b,
		0x034401a9,
		0x02230149,
		0x013900eb,
		0x00880094,
		0x000e0044,
		0xffc8fffe,
		0xffafffc6,
		0xffb9ff9d,
		0xffddff84,
		0x000cff7f,
		0x0037ff8e,
		0x0050ffaf,
		0x0049ffe0,
		0x001b001f,
		0xffbb006b,
		0xff2800bf,
		0xfe59011a,
		0xfd530179,
		0xfc1b01d9,
		0xfaae023b,
		0x04540192,
		0x03020141,
		0x01e200f0,
		0x00fa00a2,
		0x004e0058,
		0xffda0015,
		0xff9dffda,
		0xff8dffab,
		0xffa3ff87,
		0xffd3ff73,
		0x000fff6f,
		0x0047ff7b,
		0x006cff97,
		0x0070ffc0,
		0x004bfff7,
		0xfff30036,
		0xff63007c,
		0xfe9900c9,
		0xfd950118,
		0xfc5d0169,
		0xfaf101ba,
		0x041e0125,
		0x02cc00e6,
		0x01ae00a7,
		0x00c9006a,
		0x001f0030,
		0xffb1fffb,
		0xff79ffcc,
		0xff72ffa7,
		0xff91ff8b,
		0xffcbff7a,
		0x0011ff77,
		0x0054ff81,
		0x0083ff97,
		0x0090ffb8,
		0x0071ffe2,
		0x00200014,
		0xff94004c,
		0xfecc0088,
		0xfdca00c7,
		0xfc930106,
		0xfb270144,
		0x03f700c5,
		0x02a50098,
		0x0187006b,
		0x00a30040,
		0xfffc0017,
		0xff91fff2,
		0xff5effd0,
		0xff5dffb6,
		0xff83ffa1,
		0xffc5ff96,
		0x0013ff93,
		0x005eff9b,
		0x0095ffab,
		0x00a9ffc2,
		0x008fffe0,
		0x00410004,
		0xffb8002b,
		0xfef30055,
		0xfdf10082,
		0xfcbb00af,
		0xfb4f00db,
		0x03dc006b,
		0x028b0052,
};

MMP_ULONG m_ulDeltaMemD_128_255_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x016d0038,
		0x008a0020,
		0xffe50009,
		0xff7dfff4,
		0xff4cffe1,
		0xff4fffd2,
		0xff79ffc6,
		0xffc1ffc0,
		0x0015ffbf,
		0x0065ffc2,
		0x00a0ffcc,
		0x00b8ffd9,
		0x00a2ffea,
		0x0057ffff,
		0xffd00014,
		0xff0c002c,
		0xfe0b0045,
		0xfcd5005e,
		0xfb690077,
		0x03d10015,
		0x02800010,
		0x0162000b,
		0x00800006,
		0xffdb0002,
		0xff74fffd,
		0xff45fffa,
		0xff49fff7,
		0xff76fff5,
		0xffbffff3,
		0x0016fff3,
		0x0068fff4,
		0x00a5fff6,
		0x00bffff8,
		0x00aafffb,
		0x0060ffff,
		0xffda0004,
		0xff160008,
		0xfe16000e,
		0xfce00012,
		0xfb740017,
		0x03d5ffc0,
		0x0283ffd0,
		0x0166ffdf,
		0x0083ffee,
		0xffdffffb,
		0xff770008,
		0xff480013,
		0xff4b001c,
		0xff770023,
		0xffc00027,
		0x00150028,
		0x00670025,
		0x00a40020,
		0x00bc0018,
		0x00a7000e,
		0x005d0002,
		0xffd6fff4,
		0xff13ffe7,
		0xfe13ffd7,
		0xfcdcffc8,
		0xfb70ffb9,
		0x03e8ff69,
		0x0296ff8c,
		0x0178ffaf,
		0x0095ffd1,
		0xffeffff1,
		0xff86000f,
		0xff540029,
		0xff55003e,
		0xff7e004d,
		0xffc30056,
		0x00140059,
		0x00630053,
		0x009b0046,
		0x00b10034,
		0x009a001d,
		0x004d0000,
		0xffc6ffe2,
		0xff01ffc0,
		0xfe00ff9d,
		0xfccaff7a,
		0xfb5eff58,
		0x0409ff0b,
		0x02b7ff42,
		0x0199ff78,
		0x00b5ffac,
		0x000dffde,
		0xffa0000c,
		0xff6a0034,
		0xff660054,
		0xff89006c,
		0xffc9007a,
		0x0012007d,
		0x00590075,
		0x008d0061,
		0x009d0045,
		0x00820021,
		0x0031fff6,
		0xffa8ffc6,
		0xfee1ff92,
		0xfddfff5d,
		0xfca8ff27,
		0xfb3cfef0,
		0x0438fea6,
		0x02e5feee,
		0x01c6ff37,
		0x00e0ff7c,
		0x0035ffbd,
		0xffc4fffb,
		0xff8a002f,
		0xff7f005a,
		0xff99007a,
		0xffcf008d,
		0x00110090,
		0x004f0085,
		0x0078006c,
		0x00820046,
		0x005f0016,
		0x000bffdd,
		0xff7dff9e,
		0xfeb4ff5a,
		0xfdb1ff13,
		0xfc7afecb,
		0xfb0efe82,
		0x0474fe34,
		0x0321fe8d,
		0x0201fee6,
		0x0118ff3c,

};

MMP_ULONG m_ulDeltaMemD_256_335_FHD[LDC_DELTA_ARRAY_SIZE] =
{
		0x006aff8d,
		0xfff4ffd6,
		0xffb10017,
		0xff9e004b,
		0xffae0072,
		0xffd80088,
		0x000e008c,
		0x003f007e,
		0x005e0061,
		0x005e0033,
		0x0034fff8,
		0xffd8ffb2,
		0xff46ff65,
		0xfe7bff12,
		0xfd76feba,
		0xfc3efe61,
		0xfad1fe08,
		0x04bdfdb3,
		0x036afe1c,
		0x0248fe84,
		0x015cfee8,
		0x00a8ff46,
		0x002bff9c,
		0xffe0ffe7,
		0xffc20022,
		0xffc6004f,
		0xffe30069,
		0x000a006e,
		0x002e005e,
		0x0040003b,
		0x00340007,
		0x0000ffc3,
		0xff9dff73,
		0xff06ff19,
		0xfe35feb6,
		0xfd2efe50,
		0xfbf5fde8,
		0xfa89fd7f,
		0x050ffd23,
		0x03bbfd9a,
		0x0297fe0f,
		0x01a7fe80,
		0x00effee9,
		0x006aff4a,
		0x0015ff9d,
		0xffebffdf,
		0xffe10010,
		0xffef002d,
		0x00070032,
		0x001b0021,
		0x001efffa,
		0x0005ffc0,
		0xffc7ff75,
		0xff5aff1b,
		0xfebcfeb7,
		0xfde8fe48,
		0xfcdefdd5,
		0xfba3fd5f,
		0xfa36fce8,
		0x056cfc81,
		0x0416fd04,
		0x02effd85,
		0x01fafe02,
		0x013cfe74,
		0x00aefedc,
		0x004eff36,
		0x0017ff7e,
		0xffffffb3,
		0xfffbffd2,
		0x0002ffd8,
		0x0005ffc5,
		0xfff8ff9b,
		0xffd2ff5d,
		0xff87ff0b,
		0xff12feaa,
		0xfe6cfe3c,
		0xfd92fdc4,
		0xfc84fd45,
		0xfb47fcc4,
		0xf9d9fc40,
};