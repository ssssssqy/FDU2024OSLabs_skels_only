#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <linux/userfaultfd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define IP "10.20.26.33"
#define PORT 12138

const uint8_t *id = "22307140095";
const size_t page_size = 4096;
const size_t file_size = 4096 * page_size; // Access a 16MB file.
const uint32_t modulus[5] = {1787, 2339, 3449, 1223, 3581};

uint64_t file_base = 0;

struct local_file
{
    uint8_t *file;
    size_t size;
};

static void *workload(void *args)
{
    struct local_file *f = (struct local_file *)args;
    uint64_t id_num = strtoull((const char *)id, NULL, 10);

    /**
     * The worker accesses five specific pages, with page numbers derived
     * from the associated UIS account. Since `file` is not mapped, this will
     * inevitably cause page faults. Once the page faults are resolved,
     * each page's content is printed.
     */
    for (int i = 0; i < 5; i++)
    {
        uint32_t page_no = id_num % modulus[i];
        char *content = f->file + page_no * page_size;
        for (int j = 0; j < 100; j++)
        {
            printf("%c", content[j]);
        }
        printf("\n");
    }

    exit(0);
    return NULL;
}

/**
 * Fetches a specified remote page from a server and stores it in the
 * provided buffer.
 *
 * This function establishes a TCP connection with a server, sends a request
 * for a specific page number, and retrieves the page data from the server. It
 * reads the data into the provided buffer if successful, ensuring that the
 * connection is properly opened and closed. The page request is formatted as
 * "GET <id> <page_no>", where `id` is your UIS id.
 *
 * @param[out] page     The buffer where the fetched page data will be stored.
 * @param[in]  page_no  The page number to request from the remote server.
 */
static void fetch_remote_page(uint8_t *page, uint32_t page_no)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        fprintf(stderr, "Failed to create socket.\n");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address/Address not supported.\n");
        close(socket_fd);
        return;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "Connection failed.\n");
        close(socket_fd);
        return;
    }

    /* Prepare the request body. */
    char request[32];
    snprintf(request, 32, "GET %s %d", id, page_no);

    send(socket_fd, request, strlen(request), 0);

    int bytes_read = read(socket_fd, page, page_size);
    if (bytes_read != page_size)
    {
        fprintf(stderr, "Unable to read enough bytes from server\n");
        return;
    }

    close(socket_fd);
}

static void handle_pagefault(int uffd, struct uffd_msg *uffd_msg)
{
    if (uffd_msg->event != UFFD_EVENT_PAGEFAULT)
    {
        fprintf(stderr, "Unknown event on userfaultfd.\n");
        exit(EXIT_FAILURE);
    }

    printf("\e[0;32mPage fault detected at %lx\e[0m\n",
           (uint64_t)uffd_msg->arg.pagefault.address);

    /**
     * @todo Fetch the missing page from the server using `fetch_remote_page`.
     * Determine the two arguments required:
     * - The destination buffer for the page content
     * * - The page number, calculated based on the faulting address and page size
     */

    uint64_t fault_addr = (uint64_t)uffd_msg->arg.pagefault.address;
    uint32_t page_no = (fault_addr - (uint64_t)file_base) / page_size;

    uint8_t *page = malloc(page_size);
    if (!page)
    {
        fprintf(stderr, "Failed to allocate memory for page.\n");
        exit(EXIT_FAILURE);
    }

    fetch_remote_page(page, page_no);

    /* TODO END */

    struct uffdio_copy uffdio_copy;
    memset(&uffdio_copy, 0, sizeof(uffdio_copy));
    uffdio_copy.src = (uint64_t)page;
    uffdio_copy.dst = file_base + page_no * 4096;
    uffdio_copy.len = page_size;
    ioctl(uffd, UFFDIO_COPY, &uffdio_copy);
    if (uffdio_copy.copy != page_size)
    {
        fprintf(stderr, "Data only filled %" PRId64 "bytes \n",
                (long int)uffdio_copy.copy);
        exit(EXIT_FAILURE);
    }

    free(page);
}

static int init_userfaultfd(uint8_t *addr, int len)
{
    int uffd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK);

    /* Initialize userfaultfd, check for requested features. */
    struct uffdio_api uffdio_api;
    memset(&uffdio_api, 0, sizeof(uffdio_api));
    uffdio_api.api = UFFD_API;
    ioctl(uffd, UFFDIO_API, &uffdio_api);

    /* Register userfaultfd handler for addr region. */
    struct uffdio_register uffdio_register;
    memset(&uffdio_register, 0, sizeof(uffdio_register));

    /**
     * @todo Configure the three fields of `uffdio_register`: `start`, `len`, and `mode`.
     * For detailed information on these fields and their usage, refer to the documentation at:
     * https://man7.org/linux/man-pages/man2/UFFDIO_REGISTER.2const.html
     */

    uffdio_register.range.start = (uint64_t)addr;
    uffdio_register.range.len = (uint64_t)len;
    uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;


    /* TODO END */

    ioctl(uffd, UFFDIO_REGISTER, &uffdio_register);

    return uffd;
}

int main(int argc, char *argv[])
{
    /**
     * Create a private anonymous mapping. The memory will be
     * demand-zero paged--that is, not yet allocated. When we
     * actually touch the memory, it will be allocated via
     * the userfaultfd.
     */
    uint8_t *file = mmap(NULL, file_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (file == MAP_FAILED)
    {
        fprintf(stderr, "Failed to map the file.\n");
        exit(EXIT_FAILURE);
    }
    file_base = (uint64_t)file;

    int uffd = init_userfaultfd(file, file_size);

    /**
     * Initialize and spawn a worker thread to read random parts of the file.
     * Since `file` is a private anonymous mapping, accessing pages will trigger
     * page faults, which will be resolved by `handle_pagefault`.
     */
    pthread_t worker;
    struct local_file f = {.file = file, .size = file_size};
    if (pthread_create(&worker, NULL, workload, (void *)&f))
    {
        fprintf(stderr, "Failed to create worker thread.\n");
        exit(EXIT_FAILURE);
    }

    /* Handle page faults. */
    for (;;)
    {
        struct pollfd pollfd;
        memset(&pollfd, 0, sizeof(pollfd));
        /**
         * @todo Initialize `fd` and `events` fields of the `pollfd` structure.
         * We use `pollfd` and the `poll` function to monitor `userfaultfd`,
         * which becomes readable when a page fault event occurs.
         */

        pollfd.fd = uffd;
        pollfd.events = POLLIN;

        /* TODO END */

        poll(&pollfd, 1, -1);

        /**
         * After `poll` returns, `userfaultfd` is readable, signaling a page fault event.
         * Read `uffd_msg` from `userfaultfd` to gather information for page fault handling.
         */
        struct uffd_msg uffd_msg;
        if (read(uffd, &uffd_msg, sizeof(uffd_msg)) == 0)
        {
            fprintf(stderr, "Failed to read from uffd\n");
            exit(1);
        }

        handle_pagefault(uffd, &uffd_msg);
    }

    munmap(file, file_size);
    return 0;
}
