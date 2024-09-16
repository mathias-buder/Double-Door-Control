import func

Import("env")

def genSoftwareVersion():
    buildVersion = func.getGitVersion()
    buildFlag = "-D GIT_VERSION_STRING=\\\"" + buildVersion + "\\\""
    print ("Git-Version: " + buildVersion)
    return (buildFlag)

env.Append(
    BUILD_FLAGS=[genSoftwareVersion()]
)