FROM archlinux:latest AS builder

RUN pacman -Syu --noconfirm && pacman -S --noconfirm mingw-w64-gcc cmake make git base-devel zip patch

WORKDIR /build

COPY . /build

RUN patch -d /build/raylib -p1 < /build/raylib.patch
RUN x86_64-w64-mingw32-g++ --version

RUN cmake -S /build/raylib -B /build/raylib/build-win \
      -DCMAKE_TOOLCHAIN_FILE=/build/toolchain-mingw.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF && \
    cmake --build /build/raylib/build-win

RUN cmake -S /build -B /build/build-win \
        -DCMAKE_TOOLCHAIN_FILE=/build/toolchain-mingw.cmake \
        -DCMAKE_CXX_FLAGS=-I/build/raylib/src/ \
        -DCMAKE_EXE_LINKER_FLAGS="-L/build/raylib/build-win/raylib" && \
    cmake --build /build/build-win --target manalter
RUN find /usr/x86_64-w64-mingw32/bin -name "*.dll"
RUN cp "/usr/x86_64-w64-mingw32/bin/libstdc++-6.dll" /build/build-win/ \
    && cp "/usr/x86_64-w64-mingw32/bin/libgcc_s_seh-1.dll" /build/build-win \
    && cp "/usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll" /build/build-win

FROM scratch AS output
COPY --from=builder /build/build-win/src/manalter.exe /out/manalter.exe
COPY --from=builder /build/build-win/libstdc++-6.dll /out/libstdc++-6.dll
COPY --from=builder /build/build-win/libgcc_s_seh-1.dll /out/libgcc_s_seh-1.dll
COPY --from=builder /build/build-win/libwinpthread-1.dll /out/libwinpthread-1.dll
