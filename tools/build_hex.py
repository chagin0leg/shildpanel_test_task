Import("env")
import os

build_dir = env.subst("$BUILD_DIR").rstrip(os.sep)
build_name = os.path.basename(build_dir)
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(
        env.subst("$OBJCOPY -O ihex $BUILD_DIR/${PROGNAME}.elf $BUILD_DIR/" + build_name + ".hex"),
        f"Creating {build_dir}/{build_name}.hex"
    )
)
