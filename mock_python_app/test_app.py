import unittest
import time 
from app import PORT

class TestPythonApp(unittest.TestCase):
    def test_port(self):
        self.assertEqual(PORT, 8081)

    def test_logic(self):
        time.sleep(2) 
        self.assertTrue(1 + 1 == 2)

if __name__ == '__main__':
    unittest.main()
