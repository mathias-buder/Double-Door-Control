import subprocess

def getGitVersion():
    """
    Retrieves the current Git version of the repository.

    This function runs the `git describe --always --long` command to get a
    description of the current commit. The output includes the most recent
    tag, the number of commits since that tag, and the abbreviated commit hash.

    Returns:
        str: A string containing the Git version description.
    """
    return subprocess.run(["git", "describe", "--always", "--long"], stdout=subprocess.PIPE, text=True).stdout.strip()