#pragma once

/* TEA is a 64-bit symmetric block cipher with a 128-bit key, developed
    by David J. Wheeler and Roger M. Needham, and described in their
    paper at <URL:http://www.cl.cam.ac.uk/ftp/users/djw3/tea.ps>.

    This implementation is based on their code in
    <URL:http://www.cl.cam.ac.uk/ftp/users/djw3/xtea.ps> 
*/

int TEA_Encrypt(DWORD *dest, const DWORD *src, const DWORD *key, int size);
int TEA_Decrypt(DWORD *dest, const DWORD *src, const DWORD *key, int size);

int GOST_Encrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector);
int GOST_Decrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector);

int DES_Encrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector);
int DES_Decrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector);
