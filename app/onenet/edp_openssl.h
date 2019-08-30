/*
 * ��˵����
 * ���ļ�����openssl��ʵ�ּӽ��ܣ�
 * �����Ҫ���ܹ�����û��openssl�⣬����Ҫ�Լ�ʵ��
 */

#ifndef _OPENSSL_20150710_69toglgn2b_H_
#define _OPENSSL_20150710_69toglgn2b_H_

#ifdef _ENCRYPT

#include "EdpKit.h"
#include <openssl/rsa.h>
#include <openssl/aes.h>

typedef enum
{
	kTypeAes = 1
} EncryptAlgType;

#define EDPKIT_DLL
/*
 * ������:  PacketEncryptReq
 * ����:    ��� ���豸���豸�Ƶ�EDPЭ���, ��������
 * ˵��:    ���ص�EDP�����͸��豸�ƺ�, ��Ҫ�ͻ�����ɾ���ð�
 *          �豸�ƻ�ظ�������Ӧ���豸
 * ��غ���:UnpackEncryptResp
 * ����:    type �ԳƼ����㷨���ͣ�Ŀǰֻ֧��AES������ΪkTypeAes

 * ����ֵ:  ���� (EdpPacket*)
 *          �ǿ�        EDPЭ���
 *          Ϊ��        EDPЭ�������ʧ��
 */
EDPKIT_DLL EdpPacket *PacketEncryptReq(EncryptAlgType type);

/*
 * ������:  UnpackEncryptResp
 * ����:    ���,����ָ���ĶԳƼ����㷨���ͳ�ʼ����Ӧ
 *          �ԳƼ����㷨
 *
 * ��غ���:PacketEncryptReq
 * ����:    pkg         EDP��, ������������Ӧ��
 * ����ֵ:  ���� (int32)
 *          =0          ���ӳɹ�
 *          >0          ����ʧ��, ����ʧ��ԭ���<OneNet���뷽����ӿ�.docx>
 *          <0          ����ʧ��, ����ʧ��ԭ�����h�ļ��Ĵ�����
 */
EDPKIT_DLL int32 UnpackEncryptResp(EdpPacket *pkg);

/*
 * ������:  SymmEncrypt
 * ����:    ��pkg��remain_len֮�������ݼ��ܣ�
 *          ����֮���������Ȼ�����pkgָʾ�Ŀռ���
 *
 * ��غ���:SymmDecrypt
 * ����:    pkg         EDP��
 * ����ֵ:  ���� (int32)
 *          >=0         ��Ҫ���ܵ����ݼ��ܺ����ݳ���
 *          <0          ����ʧ��
 */
EDPKIT_DLL int SymmEncrypt(EdpPacket *pkg);

/*
 * ������:  UnpackEncryptResp
 * ����:    ���,����ָ���ĶԳƼ����㷨���ͳ�ʼ����Ӧ
 *          �ԳƼ����㷨��key
 *
 * ��غ���:PacketEncryptReq
 * ����:    pkg         EDP��

 * ����ֵ:  ���� (int32)
 *          >=0         ��Ҫ���ܵ����ݽ��ܺ�ĳ���
 *          <0          ����ʧ��
 */
EDPKIT_DLL int SymmDecrypt(EdpPacket *pkg);

#endif
#endif
