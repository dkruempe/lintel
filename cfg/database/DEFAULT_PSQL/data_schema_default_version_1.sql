-- Seed-Daten wurden bereinigt (keine echten Personendaten/Passwörter). Beispielwerte sind absichtlich nicht-funktionsfähig.
insert into public.users(first_name, last_name, e_mail, user_name, password, created_timestamp)
values ('Example', 'User', 'example@example.com', 'example_user',
         'REPLACE_WITH_NON_FUNCTIONAL_HASH_OR_SET_PASSWORD_MANUALLY',
         '2021-09-08 17:04:49.580723+02');
insert into public.groups(name, virtual)
values ('User', false),
       ('Admin', false);
insert into public.user_groups_relation(user_name, group_name)
values ('example_user', 'User'),
       ('example_user', 'Admin');