###################################
Launch Manager Configuration Schema
###################################

This folder contains the development environment for the Launch Manager configuration JSON Schema. The schema is developed across multiple files to facilitate maintenance and modifications, then compiled into a single file for publication. This approach simplifies the development process while maintaining convenience for end users.


********
Examples
********

Configuration examples are provided in the ``examples`` folder, each accompanied by a brief description.


******
Schema
******

The ``schema`` folder contains the primary development work. The setup uses a multi-file structure where reusable ``types`` are stored in the types folder, and the top-level schema resides in the ``s-core_launch_manager.schema.json`` file. The key feature of this setup is the use of relative ``$ref`` paths throughout the folder structure. Note that all references are constructed relative to the current file's location.

For instance, to reference a file from a subfolder use:

.. code-block:: json

    "$ref": "./subfolder_name/file_name"

And to reference a file from the same folder use:

.. code-block:: json

    "$ref": "./file_name"


****************
Published Schema
****************

The official, end-user consumable schema is placed in the ``published_schema`` folder. Upon completion of development, the multi-file schema from the ``schema`` folder is merged into a single file and published here.

*******
Scripts
*******

Utility scripts for schema development are located in ``scripts`` folder:

 * bundle.py

   * Merges the multi-file schema into a single file for end-user distribution.

 * validate.py

   * Validates Launch Manager configuration instances against the schema. This script supports both single-file and multi-file schema formats.

