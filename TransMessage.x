struct gpb {
    int size;
    string data<>;
};

program DBTRANS {
    version DBTRANS_V1 {
        gpb DBACCESS(gpb) = 1;
    } = 1;
} = 0x2fffffff;

