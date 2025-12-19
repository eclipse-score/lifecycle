FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y gdb dlt-tools libtsan0 libasan6 libubsan1 llvm && \
    apt-get clean

COPY tmp/ /opt/
COPY config/ecu_logging_config.json /etc/

COPY demo.sh /opt/demo.sh
COPY toprc /root/.config/procps/toprc
COPY tsan_suppressions /opt/tsan_suppressions
RUN mv /opt/control_app/lmcontrol /usr/bin/

ENV LD_LIBRARY_PATH=/opt/lib

# std::cout logging with color highlighting
# kFatal = 0,
# kError = 1,
# kWarn = 2,
# kInfo = 3,
# kDebug = 4,
# kVerbose = 5,
ENV LC_STDOUT_LOG_LEVEL=3
ENV MW_LOG_CONFIG_FILE=/opt/etc/ecu_logging_config.json
ENV TSAN_OPTIONS="suppressions=/opt/tsan_suppressions"

RUN echo 'alias fail="kill -s SIGUSR1"' >> ~/.bashrc

WORKDIR /opt/launch_manager
ENTRYPOINT [ "/opt/launch_manager/launch_manager" ]