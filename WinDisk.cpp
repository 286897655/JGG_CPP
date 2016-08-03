#include <Windows.h>
#include "WinDisk.h"

namespace WinDisk
{
	static BOOL DOIDENTIFY_COMMAND(HANDLE devicehandle, PSENDCMDINPARAMS pSCIP,
		PSENDCMDOUTPARAMS pSCOP, BYTE command, BYTE bDriveNum,
		PDWORD bytesReturned)
	{
		// PSENDCMDINPARAMS
		pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
		pSCIP->irDriveRegs.bFeaturesReg = 0;
		pSCIP->irDriveRegs.bSectorCountReg = 1;
		pSCIP->irDriveRegs.bSectorNumberReg = 1;
		pSCIP->irDriveRegs.bCylLowReg = 0;
		pSCIP->irDriveRegs.bCylHighReg = 0;
		pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);
		pSCIP->irDriveRegs.bCommandReg = command;
		pSCIP->bDriveNumber = bDriveNum;
		pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

		return (DeviceIoControl(devicehandle, SMART_RCV_DRIVE_DATA,
			pSCIP,
			sizeof(SENDCMDINPARAMS) - 1,
			pSCOP,
			sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
			bytesReturned, NULL));
	}


	static int ReadSerialNumAsPhysicalDrive(char* serial)
	{
		BOOL ret = 0;
		for (int driver = 0; driver < 2; driver++)
		{
			char drivername[128];
			sprintf_s(drivername, "\\\\.\\PhysicalDrive%d", driver);

			// create device handle
			HANDLE devicehandle = CreateFileA(drivername,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				OPEN_EXISTING, 0, NULL);

			if (INVALID_HANDLE_VALUE != devicehandle)
			{
				GETVERSIONINPARAMS VersionParams = {0};
				DWORD               BytesReturned = 0;
				// control code SMART_GET_VERSION
				if (!DeviceIoControl(
					devicehandle,
					SMART_GET_VERSION,
					NULL,
					NULL,
					&VersionParams,
					sizeof(GETVERSIONINPARAMS),
					&BytesReturned,
					NULL))
				{
					continue;
				}

				// control code success
				if (VersionParams.bIDEDeviceMap > 0)
				{
					BYTE command = (VersionParams.bIDEDeviceMap >> driver & 0x10) ? ATAPI_ID_CMD : ID_CMD;

					SENDCMDINPARAMS scip = {0};

					BYTE outcommand[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1] = {0};

					if (!DOIDENTIFY_COMMAND(
						devicehandle,
						&scip,
						(PSENDCMDOUTPARAMS)&outcommand,
						command,
						driver,
						&BytesReturned
						))
					{
						continue;
					}

					USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS)outcommand)->bBuffer;

					int position = 0;
					for (int index = 10; index < 20; index++)
					{
						serial[position] = (char)(pIdSector[index] / 256);
						position++;

						serial[position] = (char)(pIdSector[index] % 256);
						position++;
					}
					ret = 1;
				}
				CloseHandle(devicehandle);
			}
		}
		return ret;
	}

	std::string GetSerialNum()
	{
		char serial[128] = { 0 };
		ReadSerialNumAsPhysicalDrive(serial);
		return std::string(serial);
	}
}