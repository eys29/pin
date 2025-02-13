actual_file = open("actual.out", "r")
actual = int(actual_file.readline())
expected_file = open("expected.out", "r")
expected = int(expected_file.readline())

error = abs(expected - actual) / expected
print("ERROR: " + str(error))