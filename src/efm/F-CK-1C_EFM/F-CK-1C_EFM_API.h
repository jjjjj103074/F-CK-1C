#pragma once

#ifdef F_CK_1C_EFM_EXPORTS
#define F_CK_1C_EFM_API __declspec(dllexport)
#else
#define F_CK_1C_EFM_API __declspec(dllimport)
#endif
