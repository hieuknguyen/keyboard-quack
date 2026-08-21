#ifndef UNICODE_MAP_H
#define UNICODE_MAP_H

#include <stdint.h>
#include <stdbool.h>

/* Tone marks */
#define TONE_NONE  0
#define TONE_SAC   1
#define TONE_HUYEN 2
#define TONE_HOI   3
#define TONE_NGA   4
#define TONE_NANG  5

/* === Lowercase === */
/* a */
#define VN_a         0x0061
#define VN_a_sac     0x00E1
#define VN_a_huyen   0x00E0
#define VN_a_hoi     0x1EA3
#define VN_a_ngai    0x00E3
#define VN_a_nang    0x1EA1

/* ă */
#define VN_acr       0x0103
#define VN_acr_sac   0x1EAF
#define VN_acr_huyen 0x1EB1
#define VN_acr_hoi   0x1EB3
#define VN_acr_ngai  0x1EB5
#define VN_acr_nang  0x1EB7

/* â */
#define VN_aci       0x00E2
#define VN_aci_sac   0x1EA5
#define VN_aci_huyen 0x1EA7
#define VN_aci_hoi   0x1EA9
#define VN_aci_ngai  0x1EAB
#define VN_aci_nang  0x1EAD

/* e */
#define VN_e         0x0065
#define VN_e_sac     0x00E9
#define VN_e_huyen   0x00E8
#define VN_e_hoi     0x1EBB
#define VN_e_ngai    0x1EBD
#define VN_e_nang    0x1EB9

/* ê */
#define VN_eci       0x00EA
#define VN_eci_sac   0x1EBF
#define VN_eci_huyen 0x1EC1
#define VN_eci_hoi   0x1EC3
#define VN_eci_ngai  0x1EC5
#define VN_eci_nang  0x1EC7

/* o */
#define VN_o         0x006F
#define VN_o_sac     0x00F3
#define VN_o_huyen   0x00F2
#define VN_o_hoi     0x1ECF
#define VN_o_ngai    0x00F5
#define VN_o_nang    0x1ECD

/* ô */
#define VN_oci       0x00F4
#define VN_oci_sac   0x1ED1
#define VN_oci_huyen 0x1ED3
#define VN_oci_hoi   0x1ED5
#define VN_oci_ngai  0x1ED7
#define VN_oci_nang  0x1ED9

/* ơ */
#define VN_ohr       0x01A1
#define VN_ohr_sac   0x1EDB
#define VN_ohr_huyen 0x1EDD
#define VN_ohr_hoi   0x1EDF
#define VN_ohr_ngai  0x1EE1
#define VN_ohr_nang  0x1EE3

/* u */
#define VN_u         0x0075
#define VN_u_sac     0x00FA
#define VN_u_huyen   0x00F9
#define VN_u_hoi     0x1EE7
#define VN_u_ngai    0x0169
#define VN_u_nang    0x1EE5

/* ư */
#define VN_uhr       0x01B0
#define VN_uhr_sac   0x1EE9
#define VN_uhr_huyen 0x1EEB
#define VN_uhr_hoi   0x1EED
#define VN_uhr_ngai  0x1EEF
#define VN_uhr_nang  0x1EF1

/* i */
#define VN_i         0x0069
#define VN_i_sac     0x00ED
#define VN_i_huyen   0x00EC
#define VN_i_hoi     0x1EC9
#define VN_i_ngai    0x0129
#define VN_i_nang    0x1ECB

/* y */
#define VN_y         0x0079
#define VN_y_sac     0x00FD
#define VN_y_huyen   0x1EF3
#define VN_y_hoi     0x1EF7
#define VN_y_ngai    0x1EF9
#define VN_y_nang    0x1EF5

/* đ */
#define VN_dd        0x0111

/* === Uppercase === */
/* A */
#define VN_A         0x0041
#define VN_A_sac     0x00C1
#define VN_A_huyen   0x00C0
#define VN_A_hoi     0x1EA2
#define VN_A_ngai    0x00C3
#define VN_A_nang    0x1EA0

/* Ă */
#define VN_Acr       0x0102
#define VN_Acr_sac   0x1EAE
#define VN_Acr_huyen 0x1EB0
#define VN_Acr_hoi   0x1EB2
#define VN_Acr_ngai  0x1EB4
#define VN_Acr_nang  0x1EB6

/* Â */
#define VN_Aci       0x00C2
#define VN_Aci_sac   0x1EA4
#define VN_Aci_huyen 0x1EA6
#define VN_Aci_hoi   0x1EA8
#define VN_Aci_ngai  0x1EAA
#define VN_Aci_nang  0x1EAC

/* E */
#define VN_E         0x0045
#define VN_E_sac     0x00C9
#define VN_E_huyen   0x00C8
#define VN_E_hoi     0x1EBA
#define VN_E_ngai    0x1EBC
#define VN_E_nang    0x1EB8

/* Ê */
#define VN_Eci       0x00CA
#define VN_Eci_sac   0x1EBE
#define VN_Eci_huyen 0x1EC0
#define VN_Eci_hoi   0x1EC2
#define VN_Eci_ngai  0x1EC4
#define VN_Eci_nang  0x1EC6

/* O */
#define VN_O         0x004F
#define VN_O_sac     0x00D3
#define VN_O_huyen   0x00D2
#define VN_O_hoi     0x1ECE
#define VN_O_ngai    0x00D5
#define VN_O_nang    0x1ECC

/* Ô */
#define VN_Oci       0x00D4
#define VN_Oci_sac   0x1ED0
#define VN_Oci_huyen 0x1ED2
#define VN_Oci_hoi   0x1ED4
#define VN_Oci_ngai  0x1ED6
#define VN_Oci_nang  0x1ED8

/* Ơ */
#define VN_Ohr       0x01A0
#define VN_Ohr_sac   0x1EDA
#define VN_Ohr_huyen 0x1EDC
#define VN_Ohr_hoi   0x1EDE
#define VN_Ohr_ngai  0x1EE0
#define VN_Ohr_nang  0x1EE2

/* U */
#define VN_U         0x0055
#define VN_U_sac     0x00DA
#define VN_U_huyen   0x00D9
#define VN_U_hoi     0x1EE6
#define VN_U_ngai    0x0168
#define VN_U_nang    0x1EE4

/* Ư */
#define VN_Uhr       0x01AF
#define VN_Uhr_sac   0x1EE8
#define VN_Uhr_huyen 0x1EEA
#define VN_Uhr_hoi   0x1EEC
#define VN_Uhr_ngai  0x1EEE
#define VN_Uhr_nang  0x1EF0

/* I */
#define VN_I         0x0049
#define VN_I_sac     0x00CD
#define VN_I_huyen   0x00CC
#define VN_I_hoi     0x1EC8
#define VN_I_ngai    0x0128
#define VN_I_nang    0x1ECA

/* Y */
#define VN_Y         0x0059
#define VN_Y_sac     0x00DD
#define VN_Y_huyen   0x1EF2
#define VN_Y_hoi     0x1EF6
#define VN_Y_ngai    0x1EF8
#define VN_Y_nang    0x1EF4

/* Đ */
#define VN_DD        0x0110

/* Convert any uppercase Vietnamese character to lowercase; sets *is_upper */
static inline uint32_t vn_to_lower(uint32_t cp, bool *is_upper) {
    if (is_upper) *is_upper = false;
    switch (cp) {
    case VN_A: if (is_upper) *is_upper = true; return VN_a;
    case VN_A_sac: if (is_upper) *is_upper = true; return VN_a_sac;
    case VN_A_huyen: if (is_upper) *is_upper = true; return VN_a_huyen;
    case VN_A_hoi: if (is_upper) *is_upper = true; return VN_a_hoi;
    case VN_A_ngai: if (is_upper) *is_upper = true; return VN_a_ngai;
    case VN_A_nang: if (is_upper) *is_upper = true; return VN_a_nang;

    case VN_Acr: if (is_upper) *is_upper = true; return VN_acr;
    case VN_Acr_sac: if (is_upper) *is_upper = true; return VN_acr_sac;
    case VN_Acr_huyen: if (is_upper) *is_upper = true; return VN_acr_huyen;
    case VN_Acr_hoi: if (is_upper) *is_upper = true; return VN_acr_hoi;
    case VN_Acr_ngai: if (is_upper) *is_upper = true; return VN_acr_ngai;
    case VN_Acr_nang: if (is_upper) *is_upper = true; return VN_acr_nang;

    case VN_Aci: if (is_upper) *is_upper = true; return VN_aci;
    case VN_Aci_sac: if (is_upper) *is_upper = true; return VN_aci_sac;
    case VN_Aci_huyen: if (is_upper) *is_upper = true; return VN_aci_huyen;
    case VN_Aci_hoi: if (is_upper) *is_upper = true; return VN_aci_hoi;
    case VN_Aci_ngai: if (is_upper) *is_upper = true; return VN_aci_ngai;
    case VN_Aci_nang: if (is_upper) *is_upper = true; return VN_aci_nang;

    case VN_E: if (is_upper) *is_upper = true; return VN_e;
    case VN_E_sac: if (is_upper) *is_upper = true; return VN_e_sac;
    case VN_E_huyen: if (is_upper) *is_upper = true; return VN_e_huyen;
    case VN_E_hoi: if (is_upper) *is_upper = true; return VN_e_hoi;
    case VN_E_ngai: if (is_upper) *is_upper = true; return VN_e_ngai;
    case VN_E_nang: if (is_upper) *is_upper = true; return VN_e_nang;

    case VN_Eci: if (is_upper) *is_upper = true; return VN_eci;
    case VN_Eci_sac: if (is_upper) *is_upper = true; return VN_eci_sac;
    case VN_Eci_huyen: if (is_upper) *is_upper = true; return VN_eci_huyen;
    case VN_Eci_hoi: if (is_upper) *is_upper = true; return VN_eci_hoi;
    case VN_Eci_ngai: if (is_upper) *is_upper = true; return VN_eci_ngai;
    case VN_Eci_nang: if (is_upper) *is_upper = true; return VN_eci_nang;

    case VN_O: if (is_upper) *is_upper = true; return VN_o;
    case VN_O_sac: if (is_upper) *is_upper = true; return VN_o_sac;
    case VN_O_huyen: if (is_upper) *is_upper = true; return VN_o_huyen;
    case VN_O_hoi: if (is_upper) *is_upper = true; return VN_o_hoi;
    case VN_O_ngai: if (is_upper) *is_upper = true; return VN_o_ngai;
    case VN_O_nang: if (is_upper) *is_upper = true; return VN_o_nang;

    case VN_Oci: if (is_upper) *is_upper = true; return VN_oci;
    case VN_Oci_sac: if (is_upper) *is_upper = true; return VN_oci_sac;
    case VN_Oci_huyen: if (is_upper) *is_upper = true; return VN_oci_huyen;
    case VN_Oci_hoi: if (is_upper) *is_upper = true; return VN_oci_hoi;
    case VN_Oci_ngai: if (is_upper) *is_upper = true; return VN_oci_ngai;
    case VN_Oci_nang: if (is_upper) *is_upper = true; return VN_oci_nang;

    case VN_Ohr: if (is_upper) *is_upper = true; return VN_ohr;
    case VN_Ohr_sac: if (is_upper) *is_upper = true; return VN_ohr_sac;
    case VN_Ohr_huyen: if (is_upper) *is_upper = true; return VN_ohr_huyen;
    case VN_Ohr_hoi: if (is_upper) *is_upper = true; return VN_ohr_hoi;
    case VN_Ohr_ngai: if (is_upper) *is_upper = true; return VN_ohr_ngai;
    case VN_Ohr_nang: if (is_upper) *is_upper = true; return VN_ohr_nang;

    case VN_U: if (is_upper) *is_upper = true; return VN_u;
    case VN_U_sac: if (is_upper) *is_upper = true; return VN_u_sac;
    case VN_U_huyen: if (is_upper) *is_upper = true; return VN_u_huyen;
    case VN_U_hoi: if (is_upper) *is_upper = true; return VN_u_hoi;
    case VN_U_ngai: if (is_upper) *is_upper = true; return VN_u_ngai;
    case VN_U_nang: if (is_upper) *is_upper = true; return VN_u_nang;

    case VN_Uhr: if (is_upper) *is_upper = true; return VN_uhr;
    case VN_Uhr_sac: if (is_upper) *is_upper = true; return VN_uhr_sac;
    case VN_Uhr_huyen: if (is_upper) *is_upper = true; return VN_uhr_huyen;
    case VN_Uhr_hoi: if (is_upper) *is_upper = true; return VN_uhr_hoi;
    case VN_Uhr_ngai: if (is_upper) *is_upper = true; return VN_uhr_ngai;
    case VN_Uhr_nang: if (is_upper) *is_upper = true; return VN_uhr_nang;

    case VN_I: if (is_upper) *is_upper = true; return VN_i;
    case VN_I_sac: if (is_upper) *is_upper = true; return VN_i_sac;
    case VN_I_huyen: if (is_upper) *is_upper = true; return VN_i_huyen;
    case VN_I_hoi: if (is_upper) *is_upper = true; return VN_i_hoi;
    case VN_I_ngai: if (is_upper) *is_upper = true; return VN_i_ngai;
    case VN_I_nang: if (is_upper) *is_upper = true; return VN_i_nang;

    case VN_Y: if (is_upper) *is_upper = true; return VN_y;
    case VN_Y_sac: if (is_upper) *is_upper = true; return VN_y_sac;
    case VN_Y_huyen: if (is_upper) *is_upper = true; return VN_y_huyen;
    case VN_Y_hoi: if (is_upper) *is_upper = true; return VN_y_hoi;
    case VN_Y_ngai: if (is_upper) *is_upper = true; return VN_y_ngai;
    case VN_Y_nang: if (is_upper) *is_upper = true; return VN_y_nang;

    case VN_DD: if (is_upper) *is_upper = true; return VN_dd;
    default: return cp;
    }
}

#endif