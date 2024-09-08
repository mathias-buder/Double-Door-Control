import subprocess

Import("env")

def get_git_version():
    ret = subprocess.run(["git", "describe"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
    #ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
    build_version = ret.stdout.strip()
    build_flag = "-D GIT_VERSION_STRING=\\\"" + build_version + "\\\""
    print ("Git-Version: " + build_version)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_git_version()]
)