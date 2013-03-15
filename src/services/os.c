#include <string.h>

#include "handle.h"

#include "error.h"
#include "data.h"
#include "service.h"

int nspire_os_send(nspire_handle_t *handle, void* data, size_t size) {
	int ret;
	size_t len;
	uint8_t buffer[254], *ptr = data;

	if ( (ret = service_connect(handle, 0x4080)) )
		return ret;

	if ( (ret = data_build("bw", buffer, sizeof(buffer), &len,
			0x03, (uint32_t)size)) )
		goto end;

	if ( (ret = data_write(handle, buffer, len)) )
		goto end;

	if ( (ret = data_read(handle, buffer, sizeof(buffer))) )
		goto end;

	if (buffer[0] != 0x04) {
		ret = -NSPIRE_ERR_OSFAILED;
		goto end;
	}

	buffer[0] = 0x05;
	while (size) {
		len = (253 < size) ? 253 : size;

		memcpy(buffer + 1, ptr, len);
		if ( (ret = data_write(handle, buffer, len+1)) )
			goto end;

		if (ptr == data) {
			/* First run - read 0xFF00 */
			uint16_t code;
			if ( (ret = data_read(handle, &code, 2)) )
				goto end;

			if (dcpu16(code) != 0xFF00) {
				ret = -NSPIRE_ERR_OSFAILED;
				goto end;
			}
		}

		size -= len;
		ptr += len;
	}

	while (1) {
		if ( (ret = data_read(handle, buffer, sizeof(buffer))) )
			goto end;

		if (buffer[0] == 0xFF) {
			ret = -NSPIRE_ERR_OSFAILED;
			goto end;
		}

		if (buffer[1] == 100)
			break;
	}

	ret = NSPIRE_ERR_SUCCESS;
end:
	service_disconnect(handle);
	return ret;
}
