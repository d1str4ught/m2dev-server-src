#pragma once

int GOST_Encrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector);
int GOST_Decrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector);

int DES_Encrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector);
int DES_Decrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector);
