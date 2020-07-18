#ifndef _KEYBOARDDEVICE_H_
#define _KEYBOARDDEVICE_H_

#include <add-ons/input_server/InputServerDevice.h>
#include <private/i2c/i2c.h>
#include <private/shared/AutoDeleter.h>


typedef struct i2c_hid_descriptor {
	uint16 wHIDDescLength;
	uint16 bcdVersion;
	uint16 wReportDescLength;
	uint16 wReportDescRegister;
	uint16 wInputRegister;
	uint16 wMaxInputLength;
	uint16 wOutputRegister;
	uint16 wMaxOutputLength;
	uint16 wCommandRegister;
	uint16 wDataRegister;
	uint16 wVendorID;
	uint16 wProductID;
	uint16 wVersionID;
	uint32 reserved;
} _PACKED i2c_hid_descriptor;

enum {
	tipSwitchFlag = 1 << 0,
	barrelSwitchFlag = 1 << 1,
	eraserFlag = 1 << 2,
	invertFlag = 1 << 3,
	secBarrelSwitchFlag = 1 << 4,
	inRangeFlag = 1 << 5,
};

enum {
	xMax = 21658,
	yMax = 13536,
	pressureMax = 4095
};

struct TabletPacket {
	uint16 size;
	uint8 reportId;
	uint8 flags;
	uint16 x;
	uint16 y;
	uint16 pressure;
} _PACKED;

struct TabletState {
	bigtime_t when;
	float x, y;
	float pressure;
	uint32 buttons;
	int32 clicks;
};

class TabletDevice: public BInputServerDevice
{
public:
	TabletDevice();
	~TabletDevice();

	status_t InitCheck();

	status_t Start(const char* name, void* cookie);
	status_t Stop(const char* name, void* cookie);

	status_t Control(const char* name, void* cookie, uint32 command, BMessage* message);

private:
	static int32 DeviceWatcher(void *arg);

	FileDescriptorCloser fDeviceFd;
	i2c_addr fDeviceAdr;
	i2c_hid_descriptor fDesc;
	TabletState fState;
	bigtime_t fLastClick;
	int fLastClickBtn;

	thread_id fWatcherThread;
	bool fRun;
};


extern "C" _EXPORT BInputServerDevice*	instantiate_input_device();

#endif	// _KEYBOARDDEVICE_H_
