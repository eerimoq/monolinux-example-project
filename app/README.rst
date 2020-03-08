.. code-block:: text

   $ cd build/packages/async/examples/mqtt_client
   $ mqttools -l debug broker \
     --cafile ca.crt --certfile server.crt --keyfile server.key --no-check-hostname

.. code-block:: text

   $ python3 -m http.server 8001
