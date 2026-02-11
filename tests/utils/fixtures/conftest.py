import pytest

def pytest_addoption(parser):
    parser.addoption(
        "--image-path",
        action="store",
        required=False,
        help="Path to the image file for the target",
    )
#
# def pytest_addoption(parser):
#     parser.addoption(
#         "--lcm-path",
#         action="store",
#         required=False,
#         help="Path to the lcm executable",
#     )
#
