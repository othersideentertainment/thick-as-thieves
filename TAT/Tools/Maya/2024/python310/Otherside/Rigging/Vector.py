import math

'''
Class: Vector
'''
class Vector:
    
    @staticmethod
    def Distance(A, B):
        return math.sqrt( (B.x-A.x)**2  + (B.y-A.y)**2 + (B.z-A.z)**2 )

    @staticmethod      
    def CompareVector(v1, v2):
        limit = .01
        x = math.fabs(v1[0] - v2[0])
        y = math.fabs(v1[1] - v2[1])
        z = math.fabs(v1[2] - v2[2])
        if (x < limit and y < limit and z < limit):
            return True
        else:
            return False

    @staticmethod
    def DOT(v1, v2):
        return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z)     
    def __init__(self, input_value=[0,0,0]):
        self.x = input_value[0]
        self.y = input_value[1]
        self.z = input_value[2]    

    #addition
    def __add__(self, other):
        return Vector( [self.x + other.x, self.y + other.y, self.z + other.z] )

    #subtraction
    def __sub__(self, other):
        return Vector( [self.x - other.x, self.y - other.y, self.z - other.z] )

    #multiplication - (supports multiplication of vector by vector as well as vector by float)
    def __mul__(self, other):
        if isinstance(other, Vector):
            return Vector( [self.x * other.x, self.y * other.y, self.z * other.z] )
        else:
            return Vector( [self.x * other, self.y * other, self.z * other] )

    #division - (supports division of vector by vector as well as vector by float)
    def __div__(self, other):
        if isinstance(other, Vector):
            return Vector( [self.x / other.x, self.y / other.y, self.z / other.z] )
        else:
            return Vector( [self.x / other, self.y / other, self.z / other] )

    #normalize (create a unit vector)
    def normalize(self):
        length = self.length()
        return Vector( [(self.x / length), (self.y / length), (self.z / length)] )

    #get the length of the vector
    def length(self):
        return math.sqrt((self.x ** 2) + (self.y ** 2) + (self.z ** 2))
        
    #returns the current vector as a string
    def asString(self):
        return "(" + str(self.x) + "," + str(self.y) + "," + str(self.z) + ")"
    
    def asList(self):
        return [self.x, self.y, self.z]
    