===========================
 Stone Storage Cluster APIs
===========================

The :term:`Stone Storage Cluster` has a messaging layer protocol that enables
clients to interact with a :term:`Stone Monitor` and a :term:`Stone OSD Daemon`.
``librados`` provides this functionality to :term:`Stone Clients` in the form of
a library.  All Stone Clients either use ``librados`` or the same functionality
encapsulated in ``librados`` to interact with the object store.  For example,
``librbd`` and ``libstonefs`` leverage this functionality. You may use
``librados`` to interact with Stone directly (e.g., an application that talks to
Stone, your own interface to Stone, etc.).


.. toctree::
   :maxdepth: 2 

   Introduction to librados <librados-intro>
   librados (C) <librados>
   librados (C++) <libradospp>
   librados (Python) <python>
   libstonesqlite (SQLite) <libstonesqlite>
   object class <objclass-sdk>
