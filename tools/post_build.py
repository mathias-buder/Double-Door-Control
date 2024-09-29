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
    zipFileName = f"v{gitVersion}_{timeStamp}.zip"

    # Full path for the zip file
    zipFilePath = os.path.join(buildDir, zipFileName)

    # Create ZIP file
    with zipfile.ZipFile(zipFilePath, 'w') as zipf:
        # Add the .bin file to the zip
        binFilepath = os.path.join(buildDir, binFile)
        customBinFileName = f"{programName}_v{gitVersion}.bin"
        customBinFilepath = os.path.join(buildDir, customBinFileName)
        
        # Rename the bin file
        shutil.copy(binFilepath, customBinFilepath)
        
        # Add the renamed bin file to the zip
        zipf.write(customBinFilepath, customBinFileName)
        
        # Optionally, remove the copied file after adding to zip
        os.remove(customBinFilepath)

        # Add renesas driver files to the zip
        zipf.write(os.path.join(projectDir, "tools", "renesas_drivers.zip"), "renesas_drivers.zip")

        # Add the DFU unitilty and program_board.bat to the zip
        zipf.write(os.path.join(projectDir, "tools", "dfu-util.exe"), "dfu-util.exe")
        zipf.write(os.path.join(projectDir, "tools","program_board.bat"), "program_board.bat")

        # Add all english and german README PDF files to the zip
        readmeFiles = [
            os.path.join(projectDir, "README.pdf"),
            os.path.join(projectDir, "docs","README_de.pdf")
        ]

        # Print list
        # print("Adding the following README files to the ZIP:")
        # for readmeFile in readmeFiles:
        #     print(readmeFile)

        for readmeFile in readmeFiles:
            if os.path.exists(readmeFile):
            # Append "_en" to the english README files version
                if readmeFile.endswith("README.md"):
                    newName = "README_en.md"
                elif readmeFile.endswith("README.pdf"):
                    newName = "README_en.pdf"
                else:
                    newName = os.path.basename(readmeFile)
                # Add the file to the zip
                zipf.write(readmeFile, newName)
            else:
                print(f"Warning: README file not found: {readmeFile}")

    print(f"Build artifacts and documentation saved to: {zipFilePath}")

# Hook the script into the post-build step
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
