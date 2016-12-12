#ifndef WEBCATALOG_H
#define WEBCATALOG_H

#ifndef SERVER_URL
#define SERVER_URL          "https://coderus.openrepos.net"
#endif

#ifndef API_PATH
#define API_PATH            "pm2/api"
#endif

#define PROJECTS_PATH       "projects"
#define PROJECT_PATH        "project"
#define RATING_PATH         "rating"
#define FILES_PATH          "files"
#define MEDIA_PATH          "media"

#define CATALOG_URL         SERVER_URL"/"API_PATH
#define MEDIA_URL           SERVER_URL"/"MEDIA_PATH

#endif // WEBCATALOG_H
