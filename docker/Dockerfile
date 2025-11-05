# Use a standard DOSBox image (which is Alpine-based)
FROM classiccontainers/dosbox:latest

# Switch to root user to install packages
USER root

# Install wget using Alpine's 'apk' package manager
RUN apk update && \
    apk add --no-cache wget

# The default workdir is /home/dosbox, we'll download the file there.
WORKDIR /home/dosbox

# Download the .ima file
RUN wget -O paktool.ima "https://github.com/shadwork/C90-BigBlueDisk-Paktool/releases/latest/download/paktool_1_0_0.ima"

# Change file permissions so the non-root 'dosbox' user can read it
RUN chmod 666 paktool.ima

# Copy your custom autoexec config file into the container
COPY paktool.conf /home/dosbox/paktool.conf

# Switch back to the default user
USER 1000

# Set the command to run DOSBox, loading the default config
# AND our new config file, which will merge the autoexec commands.
CMD ["-conf", "/home/dosbox/.dosbox/dosbox-0.74.conf", "-conf", "/home/dosbox/paktool.conf"]