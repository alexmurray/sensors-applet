DIST_SUBDIRS = \
	acpi \
	eee \
	hddtemp \
        i2c-proc \
        i2c-sys \
	i8k \
	ibm-acpi \
        libsensors \
        nvidia \
	omnibook \
	pmu-sys \
	smu-sys \
	sonypi
SUBDIRS = \
	acpi \
	eee \
	hddtemp \
	i8k \
	ibm-acpi \
	omnibook \
	pmu-sys \
	smu-sys \
	sonypi


if LIBSENSORS
SUBDIRS      += libsensors
else
SUBDIRS      += i2c-proc i2c-sys
endif

if NVIDIA
SUBDIRS      += nvidia
endif
