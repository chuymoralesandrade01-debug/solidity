contract C {
    uint public x;
    uint constant public xConst = 1;
}

contract D {
    C constant cConst = C(address(1));
    C c = C(address(1));
    function() external returns (uint) constant fCConstPtr = cConst.x;
    function() external returns (uint) constant fCPtr = c.xConst;
}
