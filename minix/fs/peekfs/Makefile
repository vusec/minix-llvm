# Makefile for PeekFS server
#

.include <bsd.own.mk>

PROG=	peekfs
SRCS=	conf.c hook.c main.c root.c

CPPFLAGS+= -I${NETBSDSRCDIR}/minix -I${NETBSDSRCDIR}/minix/servers

DPADD+=	${LIBVTREEFS} ${LIBFSDRIVER} ${LIBSYS}
LDADD+=	-lvtreefs -lfsdriver -lsys

.include <minix.service.mk>
