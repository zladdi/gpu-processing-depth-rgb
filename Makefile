#.SECONDARY: %.oi

OBJS=gpu_add_test.o gpu_add.o Device.o Capture.o printMessage.o openni_device.o shader_util.o egl_x11.o rgb_processing.o gpu_processing.o cvglsl.o vector_ops.o labeling.o
BIN=gpu_add_test.bin


CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -ftree-vectorize -pipe -Wno-psabi -std=c++11 -DUSE_SHADERS -DNORMAL_FILTER_DEPTH

#LDFLAGS+=-L/usr/lib/arm-linux-gnueabihf -L../sdk_pvr/Builds/Linux/armv7hf/Lib/lib -L../sdk_pvr/Tools/OGLES2/Build/Linux_armv7hf/ReleaseX11 -lpthread -lrt -lOpenNI -lGLESv2  -lEGL -logles2tools 

#LDFLAGS+=-L./pvr_sdk/SDK_2016_R1.2/Framework/Bin/x86_64/ReleaseX11 -lpthread -lrt -lOpenNI -lstdc++ -lm -lX11 -lPVRCore  -lPVREgl -lPVRNativeGles -lPVRAssets -ldl

#LDFLAGS+=-L./pvr_sdk/SDK_2016_R1.2/Framework/Bin/x86_64/ReleaseX11 -lPVRGles -lPVRNativeGles -lPVREgl -lPVRAssets -lPVRCore -lX11 -lXau -lpthread -lrt -ldl -lOpenNI -lstdc++ -lm

LDFLAGS+=-L./pvr_sdk/SDK_2016_R1.2/Framework/Bin/x86_64/ReleaseX11 -lPVRNativeGles -lPVREgl -lEGL -lGLESv2 -lPVRAssets -lPVRCore -L/usr/lib -lX11 -lXau -lpthread -lrt -ldl -lOpenNI -lstdc++ -lm -m64 

INCLUDES+=-I./ -I../OpenNI/Include -Ipvr_sdk/SDK_2016_R1.2/External -Ipvr_sdk/SDK_2016_R1.2/Framework 

all: $(BIN) $(LIB)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations 

%.bin: $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) -Wl,--no-whole-archive -rdynamic $(LDFLAGS) 

%.a: $(OBJS)
	$(AR) r $@ $^

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)


