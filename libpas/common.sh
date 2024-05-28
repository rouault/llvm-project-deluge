case `uname -s` in
    Darwin)
        OS=macosx
        MAKE=make
        CCPREFIX="xcrun "
        DYLIB_OPT=-dynamiclib
        DYLIB_EXT=dylib
        ;;
    FreeBSD)
        OS=freebsd
        MAKE=gmake
        CCPREFIX=""
        DYLIB_OPT=-shared
        DYLIB_EXT=so
        ;;
    *)
        echo "Unsupported OS"
        exit 1
        ;;
esac

case `uname -m` in
    amd64|x86_64)
        ARCH=x86_64
        ;;
    arm64|aarch64)
        ARCH=aarch64
        ;;
    *)
        echo "Unsupported arch"
        exit 1
        ;;
esac
