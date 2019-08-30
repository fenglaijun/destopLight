#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include "EdpKit.h"
#include "mem.h"
#include "osapi.h"

/*---------------------------------------------------------------------------*/
Buffer *ICACHE_FLASH_ATTR NewBuffer()
{
	Buffer *buf = (Buffer *)os_malloc(sizeof(Buffer));
	buf->_data = (uint8 *)os_malloc(sizeof(uint8) * BUFFER_SIZE);
	buf->_write_pos = 0;
	buf->_read_pos = 0;
	buf->_capacity = BUFFER_SIZE;
	return buf;
}
void ICACHE_FLASH_ATTR DeleteBuffer(Buffer **buf)
{
	uint8 *pdata = (*buf)->_data;
	os_free(pdata);
	os_free(*buf);
	*buf = 0;
}
int32 ICACHE_FLASH_ATTR CheckCapacity(Buffer *buf, uint32 len)
{
	uint32 cap_len = buf->_capacity;
	int32 flag = 0;
	while (cap_len - buf->_write_pos < len) /* remain len < len */
	{
		cap_len = cap_len << 1;
		if (++flag > 32)
		{
			break;    /* overflow */
		}
	}
	if (flag > 32)
	{
		return -1;
	}
	if (cap_len > buf->_capacity)
	{
		uint8 *pdata = (uint8 *)os_malloc(sizeof(uint8) * cap_len);
		os_memcpy(pdata, buf->_data, buf->_write_pos);
		os_free(buf->_data);
		buf->_data = pdata;
		buf->_capacity = cap_len;
	}
	return 0;
}
void ICACHE_FLASH_ATTR ResetBuffer(Buffer *buf)
{
	buf->_write_pos = 0;
	buf->_read_pos = 0;
}
/*---------------------------------------------------------------------------*/
int32 ICACHE_FLASH_ATTR ReadByte(EdpPacket *pkg, uint8 *val)
{
	if (pkg->_read_pos + 1 > pkg->_write_pos)
	{
		return -1;
	}
	*val = pkg->_data[pkg->_read_pos];
	pkg->_read_pos += 1;
	return 0;
}
int32 ICACHE_FLASH_ATTR ReadBytes(EdpPacket *pkg, uint8 **val, uint32 count)
{
	if (pkg->_read_pos + count > pkg->_write_pos)
	{
		return -1;
	}
	*val = (char *)os_malloc(sizeof(char) * count);
	os_memcpy(*val, pkg->_data + pkg->_read_pos, count);
	pkg->_read_pos += count;
	return 0;
}
int32 ICACHE_FLASH_ATTR ReadUint16(EdpPacket *pkg, uint16 *val)
{
	uint8 msb, lsb;
	if (pkg->_read_pos + 2 > pkg->_write_pos)
	{
		return -1;
	}
	msb = pkg->_data[pkg->_read_pos];
	pkg->_read_pos++;
	lsb = pkg->_data[pkg->_read_pos];
	pkg->_read_pos++;
	*val = (msb << 8) + lsb;
	return 0;
}
int32 ICACHE_FLASH_ATTR ReadUint32(EdpPacket *pkg, uint32 *val)
{
	int32 i = 0;
	uint32 tmpval = 0;
	if (pkg->_read_pos + 4 > pkg->_write_pos)
	{
		return -1;
	}
	while (i++ < 4)
	{
		tmpval = (tmpval << 8) | (pkg->_data[pkg->_read_pos]);
		pkg->_read_pos++;
	}
	*val = tmpval;
	return 0;
}
int32 ICACHE_FLASH_ATTR ReadStr(EdpPacket *pkg, char **val)
{
	uint16 len;
	/* read str len */
	int rc = ReadUint16(pkg, &len);
	if (rc)
	{
		return rc;
	}
	if (pkg->_read_pos + len > pkg->_write_pos)
	{
		return -1;
	}
	/* copy str val */
	*val = (char *)os_zalloc(sizeof(char) * (len + 1));
	os_strncpy(*val, (const char *)(pkg->_data + pkg->_read_pos), len);
	pkg->_read_pos += len;
	return 0;
}
/*
return  0:ok  -1:error
*/
int32 ICACHE_FLASH_ATTR ReadRemainlen(EdpPacket *pkg, uint32 *len_val)
{
	uint32 multiplier = 1;
	uint32 len_len = 0;
	uint8 onebyte = 0;
	int32 rc;
	*len_val = 0;
	do
	{
		rc = ReadByte(pkg, &onebyte);
		if (rc)
		{
			return rc;
		}

		*len_val += (onebyte & 0x7f) * multiplier;
		multiplier *= 0x80;

		len_len++;
		if (len_len > 4)
		{
			return -1;//len of len more than 4;
		}
	}
	while ((onebyte & 0x80) != 0);
	return 0;
}
/*---------------------------------------------------------------------------*/
int32 ICACHE_FLASH_ATTR WriteByte(Buffer *buf, uint8 byte)
{
	//assert(buf->_read_pos == 0);
	if (CheckCapacity(buf, 1))
	{
		return -1;
	}
	buf->_data[buf->_write_pos] = byte;
	buf->_write_pos++;
	return 0;
}
int32 ICACHE_FLASH_ATTR WriteBytes(Buffer *buf, const void *bytes, uint32 count)
{
	//assert(buf->_read_pos == 0);
	if (CheckCapacity(buf, count))
	{
		return -1;
	}
	os_memcpy(buf->_data + buf->_write_pos, bytes, count);
	buf->_write_pos += count;
	return 0;
}
int32 ICACHE_FLASH_ATTR WriteUint16(Buffer *buf, uint16 val)
{
	//assert(buf->_read_pos == 0);
	return WriteByte(buf, MOSQ_MSB(val))
	       || WriteByte(buf, MOSQ_LSB(val));
}
int32 ICACHE_FLASH_ATTR WriteUint32(Buffer *buf, uint32 val)
{
	//assert(buf->_read_pos == 0);
	return WriteByte(buf, (val >> 24) & 0x00FF)
	       || WriteByte(buf, (val >> 16) & 0x00FF)
	       || WriteByte(buf, (val >> 8) & 0x00FF)
	       || WriteByte(buf, (val) & 0x00FF);
}
int32 ICACHE_FLASH_ATTR WriteStr(Buffer *buf, const char *str, uint16 length)
{
	//assert(buf->_read_pos == 0);
	return WriteUint16(buf, length)
	       || WriteBytes(buf, str, length);
}
int32 ICACHE_FLASH_ATTR WriteRemainlen(Buffer *buf, uint32 len_val)
{
	//assert(buf->_read_pos == 0);
	uint32 remaining_length = len_val;
	int32 remaining_count = 0;
	uint8 byte = 0;
	do
	{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if (remaining_length > 0)
		{
			byte = byte | 0x80;
		}
		buf->_data[buf->_write_pos++] = byte;
		remaining_count++;
	}
	while (remaining_length > 0 && remaining_count < 5);
	//assert(remaining_count != 5);
	return 0;
}
/*---------------------------------------------------------------------------*/
/* connect1 (C->S): devid + apikey */
EdpPacket *ICACHE_FLASH_ATTR PacketConnect1(const char *devid, const char *auth_key)
{
	EdpPacket *pkg = NewBuffer();
	uint32 remainlen;
	/* msg type */
	WriteByte(pkg, CONNREQ);
	/* remain len */
	remainlen = (2 + 3) + 1 + 1 + 2 + (2 + strlen(devid)) + (2 + strlen(auth_key));
	WriteRemainlen(pkg, remainlen);
	/* protocol desc */
	WriteStr(pkg, PROTOCOL_NAME, strlen(PROTOCOL_NAME));
	/* protocol version */
	WriteByte(pkg, PROTOCOL_VERSION);
	/* connect flag */
	WriteByte(pkg, 0x40);
	/* keep time */
	WriteUint16(pkg, 0x0080);
	/* DEVID */
	WriteStr(pkg, devid, strlen(devid));
	/* auth key */
	WriteStr(pkg, auth_key, strlen(auth_key));
	return pkg;
}
/* connect2 (C->S): userid + auth_info */
EdpPacket *ICACHE_FLASH_ATTR PacketConnect2(const char *userid, const char *auth_info)
{
	EdpPacket *pkg = NewBuffer();
	uint32 remainlen;
	/* msg type */
	WriteByte(pkg, CONNREQ);
	/* remain len */
	remainlen = (2 + 3) + 1 + 1 + 2 + 2 + (2 + strlen(userid)) + (2 + strlen(auth_info));
	WriteRemainlen(pkg, remainlen);
	/* protocol desc */
	WriteStr(pkg, PROTOCOL_NAME, strlen(PROTOCOL_NAME));
	/* protocol version */
	WriteByte(pkg, PROTOCOL_VERSION);
	/* connect flag */
	WriteByte(pkg, 0xC0);
	/* keep time */
	WriteUint16(pkg, 0x0080);
	/* devid */
	WriteByte(pkg, 0x00);
	WriteByte(pkg, 0x00);
	/* USERID */
	WriteStr(pkg, userid, strlen(userid));
	/* auth info */
	WriteStr(pkg, auth_info, strlen(auth_info));
	return pkg;
}
/* push_data (C->S) */
EdpPacket *ICACHE_FLASH_ATTR PacketPushdata(const char *dst_devid, const char *data, uint32 data_len)
{
	EdpPacket *pkg = NewBuffer();
	uint32 remainlen;
	/* msg type */
	WriteByte(pkg, PUSHDATA);
	/* remain len */
	remainlen = (2 + strlen(dst_devid)) + data_len;
	WriteRemainlen(pkg, remainlen);
	/* dst devid */
	WriteStr(pkg, dst_devid, strlen(dst_devid));
	/* data */
	WriteBytes(pkg, data, data_len);
	return pkg;
}
/* save_data field string */
EdpPacket *ICACHE_FLASH_ATTR PacketSaveField(const char *dst_devid,
	const char *field)
{
	EdpPacket *pkg = NewBuffer();
	uint32 remainlen;

	WriteByte(pkg, SAVEDATA);
	if (dst_devid)
	{
		/* remain len */
		remainlen = 1 + (2 + os_strlen(dst_devid)) + 1 + (2 + os_strlen(field));
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x80);
		/* dst devid */
		WriteStr(pkg, dst_devid, os_strlen(dst_devid));
	}
	else
	{
		/* remain len */
		remainlen = 1 + 1 + (2 + os_strlen(field));
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x00);
	}
	/* field string flag */
	WriteByte(pkg, 0x05);
	WriteStr(pkg, field, os_strlen(field));
	return pkg;
}
/* sava_data (C->S) */
EdpPacket *ICACHE_FLASH_ATTR PacketSaveJson(const char *dst_devid,
	char *json_str)
{
	EdpPacket *pkg = NewBuffer();
	uint32 remainlen;
//	char *json_out = (char *)os_malloc(jsonSize);
//	json_ws_send(json_obj, path, json_out);
	uint32 json_len = os_strlen(json_str);

	/* msg type */
	WriteByte(pkg, SAVEDATA);
	if (dst_devid)
	{
		/* remain len */
		remainlen = 1 + (2 + os_strlen(dst_devid)) + 1 + (2 + json_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x80);
		/* dst devid */
		WriteStr(pkg, dst_devid, os_strlen(dst_devid));
	}
	else
	{
		/* remain len */
		remainlen = 1 + 1 + (2 + json_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x00);
	}
	/* json flag */
	WriteByte(pkg, 0x01);
	/* json */
	WriteStr(pkg, json_str, json_len);
	return pkg;
}

/* sava_data (C->S) , jsontree --> string */
EdpPacket *ICACHE_FLASH_ATTR PacketSavedataJson(const char *dst_devid,
	char *json_str)
{
	EdpPacket *pkg ;//= NewBuffer();
	uint32 remainlen;
//	char *json_out = (char *)os_malloc(jsonSize);
//	json_ws_send(json_obj, path, json_out);
	uint32 json_len = os_strlen(json_str);

	/* msg type */
	WriteByte(pkg, SAVEDATA);
	if (dst_devid)
	{
		/* remain len */
		remainlen = 1 + (2 + os_strlen(dst_devid)) + 1 + (2 + json_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x80);
		/* dst devid */
		WriteStr(pkg, dst_devid, os_strlen(dst_devid));
	}
	else
	{
		/* remain len */
		remainlen = 1 + 1 + (2 + json_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x00);
	}
	/* json flag */
	WriteByte(pkg, 0x01);
	/* json */
	WriteStr(pkg, json_str, json_len);
	return pkg;
}
/* sava_data bin (C->S) */
EdpPacket *ICACHE_FLASH_ATTR PacketSavedataBin(const char *dst_devid,
	char *json_str, uint8 *bin_data, uint32 bin_len)
{
	EdpPacket *pkg;
	uint32 remainlen;
//	char *json_out = (char *)os_malloc(jsonSize);
//	json_ws_send(json_obj, path, json_out);
	uint32 json_len = os_strlen(json_str);

	/* check arguments */
//	char *desc_out = cJSON_Print(desc_obj);
//	uint32 desc_len = strlen(desc_out);
	if (json_len > (0x01 << 16) || bin_len > (3 * (0x01 << 20)) /* desc < 2^16 && bin_len < 3M*/
		)
//	        || cJSON_GetObjectItem(desc_obj, "ds_id") == 0)  /* desc_obj MUST has ds_id */
	{
//		os_free(json_out);
		return 0;
	}
	pkg = NewBuffer();
	/* msg type */
	WriteByte(pkg, SAVEDATA);
	if (dst_devid)
	{
		/* remain len */
		remainlen = 1 + (2 + os_strlen(dst_devid)) + 1 + (2 + json_len) + (4 + bin_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x80);
		/* dst devid */
		WriteStr(pkg, dst_devid, os_strlen(dst_devid));
	}
	else
	{
		/* remain len */
		remainlen = 1 + 1 + (2 + json_len) + (4 + bin_len);
		WriteRemainlen(pkg, remainlen);
		/* translate address flag */
		WriteByte(pkg, 0x00);
	}
	/* bin flag */
	WriteByte(pkg, 0x02);
	/* desc */
	WriteStr(pkg, json_str, json_len);
//	os_free(json_out);
	/* bin data */
	WriteUint32(pkg, bin_len);
//	WriteBytes(pkg, bin_data, bin_len)
	return pkg;
}
EdpPacket *ICACHE_FLASH_ATTR PacketCmdresp(const char *cmdid, uint8 *cmdbody, uint32 bodylen)
{
	EdpPacket *pkt = NewBuffer();
	uint32 remainlen;
	WriteByte(pkt, CMDRESP);
	remainlen = 2 + os_strlen(cmdid) + 4 + bodylen;
	WriteRemainlen(pkt, remainlen);
	WriteStr(pkt, cmdid, os_strlen(cmdid));
	WriteUint32(pkt, bodylen);
	WriteBytes(pkt, cmdbody, bodylen);
	return pkt;
}
/* ping (C->S) */
EdpPacket *ICACHE_FLASH_ATTR PacketPing(void)
{
	EdpPacket *pkg = NewBuffer();
	/* msg type */
	WriteByte(pkg, PINGREQ);
	/* remain len */
	WriteRemainlen(pkg, 0);
	return pkg;
}
/*
EdpPacket *ICACHE_FLASH_ATTR PacketEncryptresp(uint8 *secret_key, uint32 len)
{
	EdpPacket *pkt = NewBuffer();
	WriteByte(pkt, ENCRYPTRESP);
	WriteRemainlen(pkt, len+2);
	WriteUint16(pkt, len);
	WriteBytes(pkt, secret_key, len);
	return pkt;
}
*/
/*---------------------------------------------------------------------------*/
/* recv stream to a edp packet (S->C) */
EdpPacket *ICACHE_FLASH_ATTR GetEdpPacket(RecvBuffer *buf)
{
	int32 flag;
	//assert(buf->_read_pos == 0);
	EdpPacket *pkg = 0;

	flag = IsPkgComplete(buf->_data, buf->_write_pos);
	if (flag <= 0)
	{
		return pkg;
	}
	pkg = NewBuffer();
	WriteBytes(pkg, buf->_data, flag);
	/* shrink buffer */
	memmove(buf->_data, buf->_data + flag, buf->_write_pos - flag);
	buf->_write_pos -= flag;
	return pkg;
}
/* is the recv buffer has a complete edp packet? */
int32 ICACHE_FLASH_ATTR IsPkgComplete(const char *data, uint32 data_len)
{

	/* recevie remaining len */
	uint32 multiplier = 0;
	uint32 len_val = 0;
	uint32 len_len = 0;
	const char *pdigit = 0;
	uint32 pkg_total_len = 0;

	if (data_len <= 1)
	{
		return 0;   /* continue receive */
	}

	multiplier = 1;
	len_val = 0;
	len_len = 1;
	pdigit = data;

	do
	{
		if (len_len > 4)
		{
			return -1;  /* protocol error; */
		}
		if (len_len > data_len - 1)
		{
			return 0;   /* continue receive */
		}
		len_len++;
		pdigit++;
		len_val += ((*pdigit) & 0x7f) * multiplier;
		multiplier *= 0x80;
	}
	while (((*pdigit) & 0x80) != 0);

	pkg_total_len = len_len + len_val;
	/* receive payload */
	if (pkg_total_len <= (uint32)data_len)
	{
#if 0
		os_printf("[%s] a complete packet len:%d\n", __func__, pkg_total_len);
#endif
		return pkg_total_len;   /* all data for this pkg is read */
	}
	else
	{
		return 0;   /* continue receive */
	}
}
/* get edp packet type, client should use this type to invoke Unpack??? function */
uint8 ICACHE_FLASH_ATTR EdpPacketType(EdpPacket *pkg)
{
	uint8 mtype = 0x00;
	ReadByte(pkg, &mtype);
	return mtype;
}
/* connect_resp (S->C)*/
int32 ICACHE_FLASH_ATTR UnpackConnectResp(EdpPacket *pkg)
{
	uint8 flag, rtn;
	uint32 remainlen;
	if (ReadRemainlen(pkg, &remainlen))
	{
		return ERR_UNPACK_CONNRESP_REMAIN;
	}
	if (ReadByte(pkg, &flag))
	{
		return ERR_UNPACK_CONNRESP_FLAG;
	}
	if (ReadByte(pkg, &rtn))
	{
		return ERR_UNPACK_CONNRESP_RTN;
	}
	//assert(pkg->_read_pos == pkg->_write_pos);
	return (int32)rtn;
}
/* push_data (S->C) */
int32 ICACHE_FLASH_ATTR UnpackPushdata(EdpPacket *pkg, char **src_devid, char **data, uint32 *data_len)
{
	uint32 remainlen;
	if (ReadRemainlen(pkg, &remainlen))
	{
		return ERR_UNPACK_PUSHD_REMAIN;
	}
	if (ReadStr(pkg, src_devid))
	{
		return ERR_UNPACK_PUSHD_DEVID;
	}
	remainlen -= (2 + strlen(*src_devid));
	if (ReadBytes(pkg, (uint8 **)data, remainlen))
	{
		return ERR_UNPACK_PUSHD_DATA;
	}
	*data_len = remainlen;
	//assert(pkg->_read_pos == pkg->_write_pos);
	return 0;
}
/* save_data (S->C) */
int32 ICACHE_FLASH_ATTR UnpackSavedata(EdpPacket *pkg, char **src_devid, uint8 *jb_flag)
{
	uint32 remainlen;
	uint8 ta_flag;
	if (ReadRemainlen(pkg, &remainlen))
	{
		return ERR_UNPACK_SAVED_REMAIN;
	}
	/* translate address flag */
	if (ReadByte(pkg, &ta_flag))
	{
		return ERR_UNPACK_SAVED_TANSFLAG;
	}
	if (ta_flag == 0x80)
	{
		if (ReadStr(pkg, src_devid))
		{
			return ERR_UNPACK_SAVED_DEVID;
		}
	}
	else
	{
		*src_devid = 0;
	}
	/* json or bin */
	if (ReadByte(pkg, jb_flag))
	{
		return ERR_UNPACK_SAVED_DATAFLAG;
	}
	return 0;
}
/* string to json */
int32 ICACHE_FLASH_ATTR UnpackSavedataJson(EdpPacket *pkg, char **json_str)
{
	if (ReadStr(pkg, json_str))
	{
		return ERR_UNPACK_SAVED_JSON;
	}
//	*json_obj = cJSON_Parse(json_str);
//	struct jsontree_context js;
//	jsontree_setup(&js, json_obj, json_putchar);
//	json_parse(&js, json_str);//在这里将会回调json_obj的每一个element
//	os_free(json_str);
	
//	if (json_obj == 0)
//	{
//		return ERR_UNPACK_SAVED_PARSEJSON;
//	}
	//assert(pkg->_read_pos == pkg->_write_pos);
	return 0;
}
int32 ICACHE_FLASH_ATTR UnpackSavedataBin(EdpPacket *pkg, char **json_str, uint8 **bin_data, uint32 *bin_len)
{
//	char *desc_str;
	if (ReadStr(pkg, json_str))
	{
		return ERR_UNPACK_SAVED_BIN_DESC;
	}
//	*desc_obj = cJSON_Parse(desc_str);
//	struct jsontree_context js;
//	jsontree_setup(&js, desc_obj, json_putchar);
//	json_parse(&js, desc_str);//在这里将会回调json_obj的每一个element
//	os_free(desc_str);

//	if (desc_obj == 0)
//	{
//		return ERR_UNPACK_SAVED_PARSEDESC;
//	}
	if (ReadUint32(pkg, bin_len))
	{
		return ERR_UNPACK_SAVED_BINLEN;
	}
	if (ReadBytes(pkg, bin_data, *bin_len))
	{
		return ERR_UNPACK_SAVED_BINDATA;
	}
	//assert(pkg->_read_pos == pkg->_write_pos);
	return 0;
}
/* ping_resp (S->C) */
int32 ICACHE_FLASH_ATTR UnpackPingResp(EdpPacket *pkg)
{
	uint32 remainlen;
	if (ReadRemainlen(pkg, &remainlen))
	{
		return ERR_UNPACK_PING_REMAIN;
	}
	//assert(pkg->_read_pos == pkg->_write_pos);
	return 0;
}
