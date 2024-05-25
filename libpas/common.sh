case `uname -s` in
    Darwin)
        OS=macosx
        MAKE=make
        ;;
    FreeBSD)
        OS=freebsd
        MAKE=gmake
        ;;
    *)
        echo Unsupported OS
        exit 1;
esac
