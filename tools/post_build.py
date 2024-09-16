import os
import func
import shutil
import zipfile
from datetime import datetime

Import("env")


# Custom post-build script for PlatformIO
def after_build(source, target, env):

    #  print ("####################### Running post-build steps for " + env.subst("$PIOENV") + " #######################")

    # Define paths
    buildDir = env.subst("$BUILD_DIR")
    binFile = env.subst("$PROGNAME") + ".bin"
    projectDir = env.subst("$PROJECT_DIR")
    programName = env.subst("$PROGNAME")

    # Check if the .hex file exists
    if not binFile:
        print("Error: No .bin file found!")
        return

    # Generate a custom ZIP file name
    gitVersion = func.getGitVersion()
    timeStamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    zipFileName = f"{gitVersion}_{timeStamp}.zip"

    # Full path for the zip file
    zipFilePath = os.path.join(buildDir, zipFileName)

    # Create ZIP file
    with zipfile.ZipFile(zipFilePath, 'w') as zipf:
        # Add the .hex file to the zip
        binFilepath = os.path.join(buildDir, binFile)
        customBinFileName = f"{programName}_{gitVersion}.bin"
        customBinFilepath = os.path.join(buildDir, customBinFileName)
        
        # Rename the bin file
        shutil.copy(binFilepath, customBinFilepath)
        
        # Add the renamed bin file to the zip
        zipf.write(customBinFilepath, customBinFileName)
        
        # Optionally, remove the copied file after adding to zip
        os.remove(customBinFilepath)

        # Add all README.md and radme.pdf files to the zip
        readmeFiles = [
            # English versions
            os.path.join(projectDir, "README.md"),
            os.path.join(projectDir, "README.pdf"),
            # German versions
            os.path.join(projectDir, "docs","README_de.md"),
            os.path.join(projectDir, "docs","README_de.pdf")
        ]

        # Print list
        # print("Adding the following README files to the ZIP:")
        # for readmeFile in readmeFiles:
        #     print(readmeFile)

        # Create a subdirectory "docs" in the zip file
        docsDir = "docs/"
        zipf.write(docsDir, docsDir)

        for readmeFile in readmeFiles:
            if os.path.exists(readmeFile):
                # Save README files in the "docs" subdirectory within the zip
                zipf.write(readmeFile, os.path.join(docsDir, os.path.basename(readmeFile)))
            else:
                print(f"Warning: README file not found: {readmeFile}")

    print(f"Build artifacts and documentation saved to: {zipFilePath}")

# Hook the script into the post-build step
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)