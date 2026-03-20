from pathlib import Path


def check_for_failures(path: Path, expected_count: int):
    """Check expected_count xml files for failures, raising an exception if
    a failure is found or a different number of xml files are found.
    """
    failing_files = []
    checked_files = []
    for file in path.iterdir():
        if file.suffix == ".xml":
            gtest_xml = open(file).read()
            query = 'failures="'
            failure_number = gtest_xml[gtest_xml.find(query) + len(query)]
            if failure_number != "0":
                failing_files.append(file.name)
            checked_files.append(file.name)
    if len(failing_files) > 0:
        raise RuntimeError(
            f"Failures found in the following files:\n {'\n'.join(failing_files)}"
        )
    if len(checked_files) != expected_count:
        raise RuntimeError(
            f"Expected to find {expected_count} xml files, instead found {len(checked_files)}:\n{'\n'.join(checked_files)}"
            f"Checked in dir {path}"
        )
