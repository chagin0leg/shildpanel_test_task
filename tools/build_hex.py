Import("env")
import os

# Получаем путь к директории сборки
build_dir = env.subst("$BUILD_DIR").rstrip(os.sep)
print(f"Путь к директории сборки: {build_dir}")

# Извлекаем имя последней директории из пути
build_name = os.path.basename(build_dir)
print(f"Имя последней директории (среды сборки): {build_name}")

# Генерация .hex файла из .elf
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(
        # Используем env.subst для подстановки переменных в команду
        env.subst("$OBJCOPY -O ihex $BUILD_DIR/${PROGNAME}.elf $BUILD_DIR/" + build_name + ".hex"),
        f"Создание {build_dir}/{build_name}.hex"
    )
)
