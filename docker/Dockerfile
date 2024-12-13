# monk OS build tools

FROM brett/gcc-cross-x86_64-elf
LABEL org.opencontainers.image.authors="Brett Vickers"

RUN set -x \
	&& apt-get update \
	&& apt-get install -y git nasm genisoimage \
	&& mkdir -p /code

VOLUME /code

CMD ["/bin/bash"]
